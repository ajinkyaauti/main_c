#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace p2p {

struct ClientInfo {
    std::string id;
    std::string ip;
    int port;
    socket_t socket;
    std::vector<std::string> files;
};

class Server {
public:
    Server(int port);
    ~Server();
    
    bool start();
    void stop();
    void run();
    
private:
    void handleClient(socket_t clientSocket);
    void processCommand(socket_t clientSocket, const std::string& command);
    void sendResponse(socket_t clientSocket, const std::string& response);
    std::string receiveData(socket_t clientSocket);
    
    void handleConnect(socket_t clientSocket, const std::string& peerId);
    void handleList(socket_t clientSocket);
    void handleUpload(socket_t clientSocket, const std::string& filename, size_t filesize);
    void handleDownload(socket_t clientSocket, const std::string& filename);
    void handleDisconnect(socket_t clientSocket);
    
    int port_;
    socket_t serverSocket_;
    bool running_;
    std::map<socket_t, ClientInfo> clients_;
    std::mutex clientsMutex_;
    
    bool initializeWinsock();
    void cleanupWinsock();
};

} // namespace p2p

#endif // SERVER_H
