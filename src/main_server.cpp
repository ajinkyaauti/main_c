#include "server.h"
#include "peer_file_server.h"
#include <iostream>
#include <csignal>
#include <memory>
#include <thread>

std::unique_ptr<p2p::Server> server;
std::unique_ptr<p2p::PeerFileServer> peerFileServer;

void signalHandler(int signal) {
    std::cout << "\nShutting down server..." << std::endl;
    if (server) {
        server->stop();
    }
    if (peerFileServer) {
        peerFileServer->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    int transferPort = 8081;

    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number. Using default port 8080." << std::endl;
            port = 8080;
        }
    }
    if (argc > 2) {
        transferPort = std::atoi(argv[2]);
        if (transferPort <= 0 || transferPort > 65535) {
            std::cerr << "Invalid transfer port number. Using default port 8081." << std::endl;
            transferPort = 8081;
        }
    }
    
    std::cout << "=== P2P File Transfer Server ===" << std::endl;
    std::cout << "Starting server on port " << port << "..." << std::endl;
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signalHandler);
#ifndef _WIN32
    signal(SIGTERM, signalHandler);
#endif
    
    server = std::make_unique<p2p::Server>(port);

    if (!server->start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    peerFileServer = std::make_unique<p2p::PeerFileServer>(transferPort, "uploads");
    if (!peerFileServer->start()) {
        std::cerr << "Failed to start peer file transfer server on port "
                   << transferPort << std::endl;
        server->stop();
        return 1;
    }
    std::thread peerFileServerThread([&]() { peerFileServer->run(); });
    peerFileServerThread.detach();
    std::cout << "Peer file transfer server started on port " << transferPort << std::endl;

    server->setTransferInfo(transferPort, [](const std::string& filename) {
        return peerFileServer->issueToken(filename);
    });
    
    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
    std::cout << "\nSupported commands:" << std::endl;
    std::cout << "  CONNECT <peer_id>          - Register with server" << std::endl;
    std::cout << "  LIST                       - List all available files" << std::endl;
    std::cout << "  UPLOAD <filename> <size>   - Register uploaded file" << std::endl;
    std::cout << "  DOWNLOAD <filename>        - Find file location" << std::endl;
    std::cout << "  DELETE <filename>          - Remove a shared file" << std::endl;
    std::cout << "  PUBLIC <filename>          - Make a file visible" << std::endl;
    std::cout << "  PRIVATE <filename>         - Hide a file from listing" << std::endl;
    std::cout << "  DISCONNECT                 - Disconnect from server" << std::endl;
    std::cout << std::endl;
    
    server->run();
    
    return 0;
}
