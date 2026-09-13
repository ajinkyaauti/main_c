#ifndef PEER_FILE_SERVER_H
#define PEER_FILE_SERVER_H

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <openssl/ssl.h>

#include "file_manager.h"
#include "net_compat.h"
#include "tls_context.h"

namespace p2p {

// Peer-to-peer binary transfer listener, separate from the tracker's
// command socket. Serves only files inside the configured storage root.
class PeerFileServer {
public:
    PeerFileServer(int port, std::filesystem::path storageRoot,
                   int maxConnections = 16,
                   std::filesystem::path certDirectory = "certs");
    ~PeerFileServer();

    bool start();
    void stop();
    void run();

    // False when the TLS certificate/context could not be initialized;
    // start() will fail rather than ever falling back to plaintext.
    bool isTlsReady() const { return tlsContext_.isValid(); }

    // Issues a short-lived token scoped to one file; required by GET requests.
    std::string issueToken(const std::string& filename,
                            std::chrono::seconds ttl = std::chrono::seconds(300));

private:
    void handleConnection(socket_t clientSocket);
    void sendError(SSL* ssl, const std::string& message);
    bool receiveRequestLine(SSL* ssl, std::string& line);
    void sendAll(SSL* ssl, const char* data, std::size_t length);
    bool validateToken(const std::string& token, const std::string& filename);

    bool initializeWinsock();
    void cleanupWinsock();

    struct TransferToken {
        std::string filename;
        std::chrono::steady_clock::time_point expiresAt;
    };

    int port_;
    FileManager fileManager_;
    int maxConnections_;
    std::atomic<int> activeConnections_;
    std::atomic<bool> running_;
    socket_t serverSocket_;
    std::mutex tokensMutex_;
    std::unordered_map<std::string, TransferToken> tokens_;
    TlsServerContext tlsContext_;
};

} // namespace p2p

#endif // PEER_FILE_SERVER_H
