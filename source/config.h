#pragma once

#include <filesystem>

namespace paths
{
    inline const std::filesystem::path AM_PROJECT_DIR_PATH              = AM_PROJECT_SOURCE_DIR;
    inline const std::filesystem::path AM_PROJECT_SOURCE_CODE_DIR_PATH  = AM_PROJECT_SOURCE_CODE_DIR;
    inline const std::filesystem::path AM_PROJECT_SHADERS_DIR_PATH      = AM_PROJECT_SHADERS_DIR;
    inline const std::filesystem::path AM_PROJECT_BINARY_DIR_PATH       = AM_PROJECT_BINARY_DIR;
    inline const std::filesystem::path AM_PROJECT_CONFIG_DIR_PATH       = AM_PROJECT_CONFIG_DIR;

    inline const std::filesystem::path AM_SHADER_CACHE_DIR_PATH         = AM_PROJECT_BINARY_DIR_PATH / "shader_cache";

    inline const std::filesystem::path AM_PROJECT_CONFIG_FILE_PATH  = AM_PROJECT_CONFIG_DIR_PATH / "app_config.json";
}