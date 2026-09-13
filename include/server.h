#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <functional>

#include "net_compat.h"

namespace p2p {

struct ClientInfo {
    std::string id;
    std::string ip;
    int port;
    socket_t socket;
    std::vector<std::string> files;
    std::map<std::string, bool> fileVisibility;
};

class Server {
public:
    Server(int port);
    ~Server();
    
    bool start();
    void stop();
    void run();

    // Lets DOWNLOAD responses include a peer transfer port and an issued
    // token, obtained from the peer file server that actually holds the bytes.
    void setTransferInfo(int transferPort,
                         std::function<std::string(const std::string&)> tokenIssuer);

private:
    void handleClient(socket_t clientSocket);
    void processCommand(socket_t clientSocket, const std::string& command);
    void sendResponse(socket_t clientSocket, const std::string& response);
    
    void handleConnect(socket_t clientSocket, const std::string& peerId);
    void handleList(socket_t clientSocket);
    void handleUpload(socket_t clientSocket, const std::string& filename, size_t filesize);
    void handleDownload(socket_t clientSocket, const std::string& filename);
    void handleDelete(socket_t clientSocket, const std::string& filename);
    void handleVisibility(socket_t clientSocket, const std::string& filename, bool isPublic);
    void handleDisconnect(socket_t clientSocket);
    
    int port_;
    socket_t serverSocket_;
    bool running_;
    std::map<socket_t, ClientInfo> clients_;
    std::mutex clientsMutex_;
    int transferPort_;
    std::function<std::string(const std::string&)> tokenIssuer_;

    bool initializeWinsock();
    void cleanupWinsock();
};

} // namespace p2p

#endif // SERVER_H
