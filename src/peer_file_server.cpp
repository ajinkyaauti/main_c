#include "peer_file_server.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

namespace p2p {

namespace {
constexpr std::size_t MAX_REQUEST_LINE = 1024;
constexpr std::size_t MAX_REQUEST_LENGTH = 4 * 1024 * 1024; // 4 MB per request
constexpr int SOCKET_TIMEOUT_SECONDS = 30;
}

PeerFileServer::PeerFileServer(int port, std::filesystem::path storageRoot,
                               int maxConnections, std::filesystem::path certDirectory)
    : port_(port), fileManager_(std::move(storageRoot)),
      maxConnections_(maxConnections), activeConnections_(0),
      running_(false), serverSocket_(INVALID_SOCKET),
      tlsContext_(std::move(certDirectory)) {
}

PeerFileServer::~PeerFileServer() {
    stop();
}

bool PeerFileServer::initializeWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void PeerFileServer::cleanupWinsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool PeerFileServer::start() {
    if (!tlsContext_.isValid()) {
        std::cerr << "Refusing to start: TLS certificate is not available" << std::endl;
        return false;
    }

    if (!initializeWinsock()) {
        return false;
    }

    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == INVALID_SOCKET) {
        cleanupWinsock();
        return false;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<unsigned short>(port_));

    if (bind(serverSocket_, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
        cleanupWinsock();
        return false;
    }

    if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
        cleanupWinsock();
        return false;
    }

    running_ = true;
    return true;
}

void PeerFileServer::stop() {
    running_ = false;

    if (serverSocket_ != INVALID_SOCKET) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }

    cleanupWinsock();
}

void PeerFileServer::run() {
    while (running_) {
        socket_t clientSocket = accept(serverSocket_, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "Peer file server accept failed" << std::endl;
            }
            continue;
        }

        if (activeConnections_ >= maxConnections_) {
            // Reject without a TLS handshake; the client sees a closed connection.
            closesocket(clientSocket);
            continue;
        }

        ++activeConnections_;
        std::thread([this, clientSocket]() {
            handleConnection(clientSocket);
            --activeConnections_;
        }).detach();
    }
}

bool PeerFileServer::receiveRequestLine(SSL* ssl, std::string& line) {
    line.clear();
    char byte = 0;

    while (line.size() < MAX_REQUEST_LINE) {
        const int received = SSL_read(ssl, &byte, 1);
        if (received <= 0) {
            return false;
        }
        if (byte == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return true;
        }
        line.push_back(byte);
    }

    return false;
}

void PeerFileServer::sendAll(SSL* ssl, const char* data, std::size_t length) {
    std::size_t sent = 0;
    while (sent < length) {
        const std::size_t remaining = length - sent;
        const int chunkSize = static_cast<int>(remaining > 64 * 1024 ? 64 * 1024 : remaining);
        const int result = SSL_write(ssl, data + sent, chunkSize);
        if (result <= 0) {
            return;
        }
        sent += static_cast<std::size_t>(result);
    }
}

void PeerFileServer::sendError(SSL* ssl, const std::string& message) {
    const std::string response = "ERROR " + message + "\n";
    sendAll(ssl, response.data(), response.size());
}

std::string PeerFileServer::issueToken(const std::string& filename, std::chrono::seconds ttl) {
    static thread_local std::mt19937_64 generator(std::random_device{}());
    std::uniform_int_distribution<std::uint64_t> distribution;

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << distribution(generator)
        << std::setw(16) << distribution(generator);
    const std::string token = oss.str();

    std::lock_guard<std::mutex> lock(tokensMutex_);
    tokens_[token] = TransferToken{filename, std::chrono::steady_clock::now() + ttl};
    return token;
}

bool PeerFileServer::validateToken(const std::string& token, const std::string& filename) {
    std::lock_guard<std::mutex> lock(tokensMutex_);
    const auto it = tokens_.find(token);
    if (it == tokens_.end()) {
        return false;
    }

    const bool valid = it->second.filename == filename &&
                        std::chrono::steady_clock::now() < it->second.expiresAt;
    if (!valid) {
        tokens_.erase(it);
    }
    return valid;
}

void PeerFileServer::handleConnection(socket_t clientSocket) {
#ifdef _WIN32
    const DWORD timeoutMs = SOCKET_TIMEOUT_SECONDS * 1000;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
#else
    timeval timeout{SOCKET_TIMEOUT_SECONDS, 0};
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif

    SSL* ssl = SSL_new(tlsContext_.handle());
    if (ssl == nullptr) {
        closesocket(clientSocket);
        return;
    }
    SSL_set_fd(ssl, static_cast<int>(clientSocket));

    if (SSL_accept(ssl) <= 0) {
        SSL_free(ssl);
        closesocket(clientSocket);
        return;
    }

    std::string requestLine;
    if (!receiveRequestLine(ssl, requestLine)) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        closesocket(clientSocket);
        return;
    }

    const auto finish = [ssl, clientSocket]() {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        closesocket(clientSocket);
    };

    std::istringstream iss(requestLine);
    std::string command;
    iss >> command;

    if (command == "GET") {
        std::string filename;
        std::size_t offset = 0;
        std::size_t length = 0;
        std::string token;

        if (!(iss >> filename >> offset >> length >> token)) {
            sendError(ssl, "Invalid request");
            finish();
            return;
        }

        if (!validateToken(token, filename)) {
            sendError(ssl, "Unauthorized");
            finish();
            return;
        }

        if (length > MAX_REQUEST_LENGTH) {
            sendError(ssl, "Requested range too large");
            finish();
            return;
        }

        std::vector<char> data;
        if (!fileManager_.readRange(filename, FileRange{offset, length}, data)) {
            sendError(ssl, "File not found or invalid range");
            finish();
            return;
        }

        const std::string header = "OK " + std::to_string(data.size()) + "\n";
        sendAll(ssl, header.data(), header.size());
        if (!data.empty()) {
            sendAll(ssl, data.data(), data.size());
        }
        finish();
        return;
    }

    if (command == "HASH") {
        std::string filename;
        std::string token;

        if (!(iss >> filename >> token)) {
            sendError(ssl, "Invalid request");
            finish();
            return;
        }

        if (!validateToken(token, filename)) {
            sendError(ssl, "Unauthorized");
            finish();
            return;
        }

        std::string hashHex;
        if (!fileManager_.computeFileHash(filename, hashHex)) {
            sendError(ssl, "File not found");
            finish();
            return;
        }

        const std::string response = "OK " + hashHex + "\n";
        sendAll(ssl, response.data(), response.size());
        finish();
        return;
    }

    sendError(ssl, "Invalid request");
    finish();
}

} // namespace p2p
