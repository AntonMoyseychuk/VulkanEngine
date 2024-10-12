#include "pch.h"

#include "file.h"

#include "utils/debug/assertion.h"


std::optional<std::vector<char>> ReadFile(const std::filesystem::path &filepath) noexcept
{
    if (!std::filesystem::exists(filepath)) {
        AM_LOG_WARN("File reading error. File {} doesn't exist.", filepath.string());
        return {};
    }

    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        AM_LOG_WARN("File reading error. Failed to open {} file.", filepath.string());
        return {};
    }

    const size_t fileSize = (size_t)file.tellg();
    std::vector<char> fileContent(fileSize);

    file.seekg(0);
    file.read(fileContent.data(), fileSize);

    file.close();

    return fileContent;
}
