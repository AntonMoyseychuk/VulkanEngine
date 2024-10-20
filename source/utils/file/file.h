#pragma once

#include <filesystem>
#include <vector>
#include <optional>

namespace fs = std::filesystem;


std::optional<std::vector<char>> ReadTextFile(const fs::path& filepath) noexcept;
std::optional<std::vector<uint8_t>> ReadBinaryFile(const fs::path& filepath) noexcept;


void WriteTextFile(const fs::path& filepath, const char* data, size_t size) noexcept;
void WriteBinaryFile(const fs::path& filepath, const uint8_t* data, size_t size) noexcept;


size_t CalculateFilesCountInDirectory(const fs::path& directoryPath) noexcept;


template <typename Func>
void ForEachSubDirectory(const fs::path& root, const Func& func) noexcept;


template <typename Func>
void ForEachFileInDirectory(const fs::path& rootDir, const Func& func) noexcept;


template <typename Func>
void ForEachFileInSubDirectories(const fs::path& rootDir, const Func& func) noexcept;


template <typename Func>
std::optional<fs::path> FindFirstFileInSubDirectoriesIf(const fs::path& rootDir, const Func& func) noexcept;


#include "file.hpp"