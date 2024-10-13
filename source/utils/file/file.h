#pragma once

#include <filesystem>
#include <vector>
#include <optional>

namespace fs = std::filesystem;

std::optional<std::vector<char>> ReadFile(const fs::path& filepath) noexcept;

template <typename Func>
void ForEachSubDirectory(const fs::path& root, const Func& func) noexcept;

template <typename Func>
void ForEachFileInSubDirectories(const fs::path& rootDir, const Func& func) noexcept;

template <typename Func>
std::optional<fs::path> FindFirstFileInSubDirectoriesIf(const fs::path& rootDir, const Func& func) noexcept;

#include "file.hpp"