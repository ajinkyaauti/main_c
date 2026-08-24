#include "server.h"
#include "protocol.h"
#include <iostream>
#include <thread>
#include <cstring>
#include <sstream>

namespace p2p {

Server::Server(int port) 
    : port_(port), serverSocket_(INVALID_SOCKET), running_(false) {
}

Server::~Server() {
    stop();
}

bool Server::initializeWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
#endif
    return true;
}

void Server::cleanupWinsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool Server::start() {
    if (!initializeWinsock()) {
        return false;
    }
    
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        cleanupWinsock();
        return false;
    }
    
    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    // Bind socket
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(serverSocket_);
        cleanupWinsock();
        return false;
    }
    
    // Listen for connections
    if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(serverSocket_);
        cleanupWinsock();
        return false;
    }
    
    running_ = true;
    std::cout << "Server started on port " << port_ << std::endl;
    return true;
}

void Server::stop() {
    running_ = false;
    
    if (serverSocket_ != INVALID_SOCKET) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }
    
    // Close all client connections
    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (auto& [socket, client] : clients_) {
        closesocket(socket);
    }
    clients_.clear();
    
    cleanupWinsock();
    std::cout << "Server stopped" << std::endl;
}

void Server::run() {
    while (running_) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        
        socket_t clientSocket = accept(serverSocket_, (sockaddr*)&clientAddr, 
#ifdef _WIN32
            &clientAddrSize
#else
            (socklen_t*)&clientAddrSize
#endif
        );
        
        if (clientSocket == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }
        
        // Get client IP
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
        
        std::cout << "New connection from " << clientIp << std::endl;
        
        // Handle client in a new thread
        std::thread([this, clientSocket]() {
            handleClient(clientSocket);
        }).detach();
    }
}

void Server::handleClient(socket_t clientSocket) {
    try {
        while (running_) {
            std::string data = receiveData(clientSocket);
            if (data.empty()) {
                break; // Client disconnected
            }
            
            processCommand(clientSocket, data);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }
    
    handleDisconnect(clientSocket);
    closesocket(clientSocket);
}

std::string Server::receiveData(socket_t clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytesReceived <= 0) {
        return "";
    }
    
    buffer[bytesReceived] = '\0';
    return std::string(buffer);
}

void Server::sendResponse(socket_t clientSocket, const std::string& response) {
    send(clientSocket, response.c_str(), response.length(), 0);
}

void Server::processCommand(socket_t clientSocket, const std::string& command) {
    auto tokens = Protocol::parseCommand(command);
    
    if (tokens.empty()) {
        sendResponse(clientSocket, Protocol::formatResponse(RESP_ERROR, "Empty command"));
        return;
    }
    
    const std::string& cmd = tokens[0];
    
    if (cmd == CMD_CONNECT && tokens.size() >= 2) {
        handleConnect(clientSocket, tokens[1]);
    } else if (cmd == CMD_LIST) {
        handleList(clientSocket);
    } else if (cmd == CMD_UPLOAD && tokens.size() >= 3) {
        size_t filesize = std::stoull(tokens[2]);
        handleUpload(clientSocket, tokens[1], filesize);
    } else if (cmd == CMD_DOWNLOAD && tokens.size() >= 2) {
        handleDownload(clientSocket, tokens[1]);
    } else if (cmd == CMD_DISCONNECT) {
        handleDisconnect(clientSocket);
    } else {
        sendResponse(clientSocket, Protocol::formatResponse(RESP_ERROR, "Unknown command"));
    }
}

void Server::handleConnect(socket_t clientSocket, const std::string& peerId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    ClientInfo info;
    info.id = peerId;
    info.socket = clientSocket;
    clients_[clientSocket] = info;
    
    std::cout << "Client registered: " << peerId << std::endl;
    sendResponse(clientSocket, Protocol::formatResponse(RESP_OK, "Connected"));
}

void Server::handleList(socket_t clientSocket) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    std::vector<std::string> allFiles;
    for (const auto& [socket, client] : clients_) {
        for (const auto& file : client.files) {
            allFiles.push_back(client.id + ":" + file);
        }
    }
    
    std::string fileList = Protocol::formatFileList(allFiles);
    sendResponse(clientSocket, Protocol::formatResponse(RESP_OK, fileList));
}

void Server::handleUpload(socket_t clientSocket, const std::string& filename, size_t filesize) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    auto it = clients_.find(clientSocket);
    if (it != clients_.end()) {
        it->second.files.push_back(filename);
        std::cout << "Client " << it->second.id << " uploaded: " << filename 
                  << " (" << filesize << " bytes)" << std::endl;
        sendResponse(clientSocket, Protocol::formatResponse(RESP_OK, "Upload registered"));
    } else {
        sendResponse(clientSocket, Protocol::formatResponse(RESP_ERROR, "Not connected"));
    }
}

void Server::handleDownload(socket_t clientSocket, const std::string& filename) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    // Find the file owner
    for (const auto& [socket, client] : clients_) {
        for (const auto& file : client.files) {
            if (file == filename) {
                sendResponse(clientSocket, Protocol::formatResponse(RESP_OK, 
                    "File found: " + client.id));
                return;
            }
        }
    }
    
    sendResponse(clientSocket, Protocol::formatResponse(RESP_ERROR, "File not found"));
}

void Server::handleDisconnect(socket_t clientSocket) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    auto it = clients_.find(clientSocket);
    if (it != clients_.end()) {
        std::cout << "Client disconnected: " << it->second.id << std::endl;
        clients_.erase(it);
    }
}

} // namespace p2p
