#pragma once

#include <filesystem>
#include <vector>
#include <optional>

namespace fs = std::filesystem;

std::optional<std::vector<char>> ReadFile(const fs::path& filepath) noexcept;

template <typename Func, typename... Args>
void ForEachFileInDirectory(const fs::path& dir, const Func& func, Args&&... args) noexcept;


#include "file.hpp"