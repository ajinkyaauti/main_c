// Simple test client for P2P File Transfer Server
// Compile: g++ -std=c++17 test_client.cpp -o test_client -lws2_32 (Windows)
//          g++ -std=c++17 test_client.cpp -o test_client -lpthread (Linux)

#include <iostream>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

constexpr size_t BUFFER_SIZE = 4096;

class SimpleClient {
public:
    SimpleClient(const std::string& host, int port) : host_(host), port_(port), socket_(INVALID_SOCKET) {}
    
    ~SimpleClient() {
        disconnect();
    }
    
    bool connect() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return false;
        }
#endif
        
        socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket_ == INVALID_SOCKET) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port_);
        inet_pton(AF_INET, host_.c_str(), &serverAddr.sin_addr);
        
        if (::connect(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Connection failed" << std::endl;
            close(socket_);
            return false;
        }
        
        std::cout << "Connected to " << host_ << ":" << port_ << std::endl;
        return true;
    }
    
    void disconnect() {
        if (socket_ != INVALID_SOCKET) {
            close(socket_);
            socket_ = INVALID_SOCKET;
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    bool sendCommand(const std::string& command) {
        std::string cmd = command + "\n";
        int sent = send(socket_, cmd.c_str(), cmd.length(), 0);
        return sent > 0;
    }
    
    std::string receiveResponse() {
        char buffer[BUFFER_SIZE];
        int received = recv(socket_, buffer, BUFFER_SIZE - 1, 0);
        if (received <= 0) {
            return "";
        }
        buffer[received] = '\0';
        return std::string(buffer);
    }
    
private:
    std::string host_;
    int port_;
    socket_t socket_;
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;
    
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
    }
    
    std::cout << "=== P2P File Transfer Test Client ===" << std::endl;
    
    SimpleClient client(host, port);
    
    if (!client.connect()) {
        return 1;
    }
    
    std::cout << "\nEnter commands (or 'quit' to exit):" << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  CONNECT myid" << std::endl;
    std::cout << "  LIST" << std::endl;
    std::cout << "  UPLOAD file.txt 1234" << std::endl;
    std::cout << "  DOWNLOAD file.txt" << std::endl;
    std::cout << "  DISCONNECT" << std::endl;
    std::cout << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        if (line.empty()) {
            continue;
        }
        
        if (line == "quit" || line == "exit") {
            break;
        }
        
        if (client.sendCommand(line)) {
            std::string response = client.receiveResponse();
            if (!response.empty()) {
                std::cout << "Server: " << response;
            } else {
                std::cout << "Connection closed by server" << std::endl;
                break;
            }
        } else {
            std::cout << "Failed to send command" << std::endl;
            break;
        }
    }
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}
