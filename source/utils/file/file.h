#pragma once

#include <filesystem>
#include <vector>
#include <optional>

#include <limits>

namespace fs = std::filesystem;


std::vector<char> ReadTextFile(const fs::path& filepath) noexcept;
void ReadTextFile(const fs::path& filepath, std::vector<char>& outData) noexcept;

std::vector<uint8_t> ReadBinaryFile(const fs::path& filepath) noexcept;
void ReadBinaryFile(const fs::path& filepath, std::vector<uint8_t>& outData) noexcept;

void WriteTextFile(const fs::path& filepath, const char* data, size_t size) noexcept;
void WriteBinaryFile(const fs::path& filepath, const uint8_t* data, size_t size) noexcept;

size_t CalculateFilesCount(const fs::path& directoryPath) noexcept;
size_t CalculateDirectoriesCount(const fs::path& directoryPath) noexcept;


// If dirTreeDepth < 0 than go throw all subdirectories
template <typename Func>
void ForEachDirectory(const fs::path& rootDir, const Func& func, uint32_t dirTreeDepth = UINT32_MAX) noexcept;


template <typename Func>
void ForEachFile(const fs::path& rootDir, const Func& func, uint32_t dirTreeDepth = UINT32_MAX) noexcept;


template <typename Func>
std::optional<fs::path> FindFirstFileIf(const fs::path& rootDir, const Func& func, uint32_t dirTreeDepth = UINT32_MAX) noexcept;


#include "file.hpp"