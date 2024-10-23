#include "pch.h"

#include "file.h"

#include "utils/debug/assertion.h"


template <typename BufferElemType>
static void ReadFileInternal(const std::filesystem::path &filepath, std::ios_base::openmode mode, std::vector<BufferElemType>& outData) noexcept
{
    if (!std::filesystem::exists(filepath)) {
        AM_LOG_WARN("File reading error. File {} doesn't exist.", filepath.string().c_str());
        return;
    }

    std::ifstream file(filepath, mode);
    if (!file.is_open()) {
        AM_LOG_WARN("File reading error. Failed to open {} file.", filepath.string().c_str());
        return;
    }

    outData.clear();

    const size_t fileSize = (size_t)file.tellg();
    if (fileSize == 0) {
        file.close();
        return;    
    }
    
    outData.resize(fileSize);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(outData.data()), fileSize);

    file.close();
}


template <typename BufferElemType>
static void WriteFileInternal(const std::filesystem::path &filepath, std::ios_base::openmode mode, const BufferElemType* data, size_t size) noexcept
{
    if (size == 0) {
        AM_LOG_WARN("File {} writing warning. data size is 0", filepath.string().c_str());
        return;
    }

    if (data == nullptr) {
        AM_LOG_WARN("File {} writing warning. data is nullptr", filepath.string().c_str());
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


std::vector<char> ReadTextFile(const std::filesystem::path &filepath) noexcept
{
    std::vector<char> fileContent;
    ReadFileInternal<char>(filepath, std::ios_base::ate, fileContent);

    return fileContent;
}


void ReadTextFile(const fs::path &filepath, std::vector<char> &outData) noexcept
{
    ReadFileInternal<char>(filepath, std::ios_base::ate, outData);
}


std::vector<uint8_t> ReadBinaryFile(const fs::path &filepath) noexcept
{
    std::vector<uint8_t> fileContent;
    ReadFileInternal<uint8_t>(filepath, std::ios_base::ate | std::ios_base::binary, fileContent);

    return fileContent;
}


void ReadBinaryFile(const fs::path &filepath, std::vector<uint8_t> &outData) noexcept
{
    ReadFileInternal<uint8_t>(filepath, std::ios_base::ate | std::ios_base::binary, outData);
}


void WriteTextFile(const fs::path & filepath, const char * data, size_t size) noexcept
{
    WriteFileInternal<char>(filepath, std::ios::out | std::ios::trunc, data, size);
}


void WriteBinaryFile(const fs::path &filepath, const uint8_t *data, size_t size) noexcept
{
    WriteFileInternal<uint8_t>(filepath, std::ios::out | std::ios::trunc | std::ios::binary, data, size);
}


size_t CalculateFilesCount(const fs::path &directoryPath) noexcept
{
    size_t fileCount = 0;
    
    for (const fs::directory_entry& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            ++fileCount;
        }
    }
    
    return fileCount;
}


size_t CalculateDirectoriesCount(const fs::path &directoryPath) noexcept
{
    size_t dirsCount = 0;
    
    for (const fs::directory_entry& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_directory()) {
            ++dirsCount;
        }
    }
    
    return dirsCount;
}
