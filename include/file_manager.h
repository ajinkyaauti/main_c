#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace p2p {

struct FileRange {
    std::size_t offset;
    std::size_t length;
};

class FileManager {
public:
    explicit FileManager(std::filesystem::path storageRoot);

    bool isSafeFilename(const std::string& filename) const;
    std::filesystem::path resolveFile(const std::string& filename) const;
    bool getFileSize(const std::string& filename, std::size_t& size) const;
    bool readRange(const std::string& filename, FileRange range,
                   std::vector<char>& data) const;

    // Streams the file in fixed-size chunks; never loads the whole file into memory.
    bool computeFileHash(const std::string& filename, std::string& hashHex) const;

private:
    std::filesystem::path storageRoot_;
};

} // namespace p2p

#endif // FILE_MANAGER_H
