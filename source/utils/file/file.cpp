#include "pch.h"

#include "file.h"

#include "utils/debug/assertion.h"


template <typename BufferElemType>
static std::optional<std::vector<BufferElemType>> ReadFileInternal(const std::filesystem::path &filepath, std::ios_base::openmode mode) noexcept
{
    if (!std::filesystem::exists(filepath)) {
        AM_LOG_WARN("File reading error. File {} doesn't exist.", filepath.string().c_str());
        return {};
    }

    std::ifstream file(filepath, mode);
    if (!file.is_open()) {
        AM_LOG_WARN("File reading error. Failed to open {} file.", filepath.string().c_str());
        return {};
    }

    const size_t fileSize = (size_t)file.tellg();
    std::vector<BufferElemType> fileContent(fileSize);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(fileContent.data()), fileSize);

    file.close();

    return fileContent;
}


template <typename BufferElemType>
static void WriteFileInternal(const std::filesystem::path &filepath, std::ios_base::openmode mode, const BufferElemType* data, size_t size) noexcept
{
    if (data == nullptr) {
        AM_LOG_WARN("File {} writing warning. data is nullptr", filepath.string().c_str());
        return;
    }

    if (size == 0) {
        AM_LOG_WARN("File {} writing warning. data size is 0", filepath.string().c_str());
        return;
    }

    std::ofstream file(filepath, mode);
    if (!file.is_open()) {
        AM_LOG_WARN("File writing error. Failed to open {} file.", filepath.string().c_str());
        return;
    }

    file.write(reinterpret_cast<const char*>(data), size);

    file.close();
}


std::optional<std::vector<char>> ReadTextFile(const std::filesystem::path &filepath) noexcept
{
    return ReadFileInternal<char>(filepath, std::ios_base::ate);
}


std::optional<std::vector<uint8_t>> ReadBinaryFile(const fs::path &filepath) noexcept
{
    return ReadFileInternal<uint8_t>(filepath, std::ios_base::ate | std::ios_base::binary);
}


void WriteTextFile(const fs::path & filepath, const char * data, size_t size) noexcept
{
    WriteFileInternal<char>(filepath, std::ios::out | std::ios::trunc, data, size);
}


void WriteBinaryFile(const fs::path &filepath, const uint8_t *data, size_t size) noexcept
{
    WriteFileInternal<uint8_t>(filepath, std::ios::out | std::ios::trunc | std::ios::binary, data, size);
}


size_t CalculateFilesCountInDirectory(const fs::path &directoryPath) noexcept
{
    size_t fileCount = 0;
    
    for (const fs::directory_entry& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            ++fileCount;
        }
    }
    
    return fileCount;
}
