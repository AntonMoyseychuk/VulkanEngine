#pragma once

#include <filesystem>

namespace fs = std::filesystem;

class PathSystem
{
public:
    static bool Init() noexcept;
    static bool IsInitialized() noexcept { return s_isInitialized; }

    static fs::path GetProjectRootDirectory() noexcept;

    static fs::path GetProjectCppSourceCodeDirectory() noexcept;
    static fs::path GetProjectShadersSourceCodeDirectory() noexcept;

    static fs::path GetProjectBinaryOutputDirectory() noexcept;
    static fs::path GetProjectShaderCacheDirectory() noexcept;
    static fs::path GetProjectShaderCacheFilepath() noexcept;

    static fs::path GetProjectConfigDirectory() noexcept;
    static fs::path GetProjectConfigFilepath() noexcept;

    PathSystem(const PathSystem& pathSys) = delete;
    PathSystem& operator=(const PathSystem& pathSys) = delete;
    
    PathSystem(PathSystem&& pathSys) = delete;
    PathSystem& operator=(PathSystem&& pathSys) = delete;

private:
    static bool PrecreateOutputDirectories() noexcept;

private:
    PathSystem() = default;

private:
    static inline fs::path s_projectRootDirPath;
    static inline fs::path s_projectCppSourceCodeDirPath;
    static inline fs::path s_projectShadersSourceCodeDirPath;
    
    static inline fs::path s_projectConfigDirPath;
    static inline fs::path s_projectConfigFilepath;
    
    static inline fs::path s_projectBinaryOutputDirPath;
    static inline fs::path s_projectShaderCacheDirPath;
    static inline fs::path s_projectShaderCacheFilepath;

    static inline bool s_isInitialized = false;
};