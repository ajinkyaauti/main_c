#include "file_manager.h"

#include <openssl/evp.h>

#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace p2p {

FileManager::FileManager(std::filesystem::path storageRoot)
    : storageRoot_(std::filesystem::weakly_canonical(std::move(storageRoot))) {
}

bool FileManager::isSafeFilename(const std::string& filename) const {
    if (filename.empty() || filename == "." || filename == ".." || filename.size() > 255) {
        return false;
    }

    for (const unsigned char character : filename) {
        if (!std::isalnum(character) && character != '_' && character != '-' && character != '.') {
            return false;
        }
    }

    return true;
}

std::filesystem::path FileManager::resolveFile(const std::string& filename) const {
    if (!isSafeFilename(filename)) {
        return {};
    }

    const auto candidate = std::filesystem::weakly_canonical(storageRoot_ / filename);
    const auto relative = candidate.lexically_relative(storageRoot_);
    if (relative.empty() || relative.is_absolute() || relative == ".." ||
        relative.begin()->string() == "..") {
        return {};
    }

    return candidate;
}

bool FileManager::getFileSize(const std::string& filename, std::size_t& size) const {
    const auto path = resolveFile(filename);
    if (path.empty() || !std::filesystem::is_regular_file(path)) {
        return false;
    }

    const auto fileSize = std::filesystem::file_size(path);
    if (fileSize > std::numeric_limits<std::size_t>::max()) {
        return false;
    }

    size = static_cast<std::size_t>(fileSize);
    return true;
}

bool FileManager::readRange(const std::string& filename, FileRange range,
                            std::vector<char>& data) const {
    std::size_t fileSize = 0;
    if (!getFileSize(filename, fileSize) || range.offset > fileSize ||
        range.length > fileSize - range.offset) {
        return false;
    }

    const auto path = resolveFile(filename);
    std::ifstream input(path, std::ios::binary);
    if (!input || range.length > std::vector<char>().max_size()) {
        return false;
    }

    input.seekg(static_cast<std::streamoff>(range.offset));
    data.resize(range.length);
    if (range.length > 0) {
        input.read(data.data(), static_cast<std::streamsize>(range.length));
        if (input.gcount() != static_cast<std::streamsize>(range.length)) {
            data.clear();
            return false;
        }
    }

    return true;
}

bool FileManager::computeFileHash(const std::string& filename, std::string& hashHex) const {
    const auto path = resolveFile(filename);
    if (path.empty() || !std::filesystem::is_regular_file(path)) {
        return false;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr || EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(context);
        return false;
    }

    std::vector<char> buffer(64 * 1024);
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytesRead = input.gcount();
        if (bytesRead > 0 &&
            EVP_DigestUpdate(context, buffer.data(), static_cast<std::size_t>(bytesRead)) != 1) {
            EVP_MD_CTX_free(context);
            return false;
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength = 0;
    if (EVP_DigestFinal_ex(context, digest, &digestLength) != 1) {
        EVP_MD_CTX_free(context);
        return false;
    }
    EVP_MD_CTX_free(context);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digestLength; ++i) {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    hashHex = oss.str();
    return true;
}

} // namespace p2p
