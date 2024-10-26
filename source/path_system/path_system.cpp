#include "pch.h"

#include "path_system.h"

#include "utils/debug/assertion.h"


bool PathSystem::Init() noexcept
{
    if (IsInitialized()) {
        AM_LOG_WARN("Path system is already initialized");
        return true;
    }

    s_projectRootDirPath                = AM_PROJECT_SOURCE_DIR;
    s_projectCppSourceCodeDirPath       = AM_PROJECT_CXX_SOURCE_CODE_DIR;
    s_projectShadersSourceCodeDirPath   = AM_PROJECT_SHADERS_SOURCE_CODE_DIR;

    s_projectConfigDirPath              = AM_PROJECT_CONFIG_DIR;
    s_projectConfigFilepath             = s_projectConfigDirPath / "app_config.json";
    
    s_projectBinaryOutputDirPath        = AM_PROJECT_BINARY_OUTPUT_DIR;
    s_projectShaderCacheDirPath         = s_projectBinaryOutputDirPath / "shader_cache";
    s_projectShaderCacheFilepath        = s_projectShaderCacheDirPath / "shader_cache.spv";

    if (!PrecreateOutputDirectories()) {
        return false;
    }

    s_isInitialized = true;

    return true;
}


fs::path PathSystem::GetProjectRootDirectory() noexcept
{
    AM_ASSERT(IsInitialized(), "Path system is not initialized");
    return s_projectRootDirPath;
}


fs::path PathSystem::GetProjectCppSourceCodeDirectory() noexcept
{
    AM_ASSERT(IsInitialized(), "Path system is not initialized");
    return s_projectCppSourceCodeDirPath;
}


fs::path PathSystem::GetProjectShadersSourceCodeDirectory() noexcept
{
    AM_ASSERT(IsInitialized(), "Path system is not initialized");
    return s_projectShadersSourceCodeDirPath;
}


fs::path PathSystem::GetProjectBinaryOutputDirectory() noexcept
{
    AM_ASSERT(IsInitialized(), "Path system is not initialized");
    return s_projectBinaryOutputDirPath;
}


fs::path PathSystem::GetProjectConfigDirectory() noexcept
{
    AM_ASSERT(IsInitialized(), "Path system is not initialized");
    return s_projectConfigDirPath;
}


fs::path PathSystem::GetProjectConfigFilepath() noexcept
{
    AM_ASSERT(IsInitialized(), "Path system is not initialized");
    return s_projectConfigFilepath;
}


fs::path PathSystem::GetProjectShaderCacheDirectory() noexcept
{
    AM_ASSERT(IsInitialized(), "Path system is not initialized");
    return s_projectShaderCacheDirPath;
}


fs::path PathSystem::GetProjectShaderCacheFilepath() noexcept
{
    AM_ASSERT(IsInitialized(), "Path system is not initialized");
    return s_projectShaderCacheFilepath;
}


bool PathSystem::PrecreateOutputDirectories() noexcept
{
    const auto CreateDirectoryIfNotExists = [](const fs::path& dirPath) -> bool
    {
        if (!fs::exists(dirPath)) {
            if (!fs::create_directory(dirPath)) {
                AM_ASSERT(false, "Failed to create {} directory", dirPath.string().c_str());
                return false;
            }
        }

        return true;
    };

    if (!CreateDirectoryIfNotExists(s_projectBinaryOutputDirPath)) {
        return false;
    }

    if (!CreateDirectoryIfNotExists(s_projectShaderCacheDirPath)) {
        return false;
    }

    return true;
}
