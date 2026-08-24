#include "protocol.h"
#include <sstream>
#include <algorithm>

namespace p2p {

std::vector<std::string> Protocol::parseCommand(const std::string& command) {
    std::vector<std::string> tokens;
    std::istringstream iss(command);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string Protocol::formatResponse(const std::string& status, const std::string& data) {
    std::string response = status;
    if (!data.empty()) {
        response += " " + data;
    }
    response += "\n";
    return response;
}

std::string Protocol::formatFileList(const std::vector<std::string>& files) {
    std::ostringstream oss;
    oss << files.size();
    for (const auto& file : files) {
        oss << " " << file;
    }
    return oss.str();
}

} // namespace p2p
