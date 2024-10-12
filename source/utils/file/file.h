#pragma once

#include <filesystem>
#include <vector>
#include <optional>

namespace fs = std::filesystem;

std::optional<std::vector<char>> ReadFile(const fs::path& filepath) noexcept;

template <typename Func, typename... Args>
void ForEachSubDirectory(const fs::path& root, const Func& func, Args&&... args) noexcept;

template <typename Func, typename... Args>
void ForEachFileInSubDirectories(const fs::path& rootDir, const Func& func, Args&&... args) noexcept;


#include "file.hpp"