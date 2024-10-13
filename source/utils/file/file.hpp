#include "file.h"


template <typename Func>
inline void ForEachSubDirectory(const fs::path &root, const Func &func) noexcept
{
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_directory()) {
            ForEachSubDirectory(entry.path(), func);
            func(entry);
        }
    }
}


template <typename Func>
inline void ForEachFileInSubDirectories(const fs::path& rootDir, const Func &func) noexcept
{
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(rootDir)) {
        if (entry.is_directory()) {
            ForEachFileInSubDirectories(entry.path(), func);
            continue;
        }
        
        func(entry);
    }
}


template <typename Func>
inline std::optional<fs::path> FindFirstFileInSubDirectoriesIf(const fs::path &rootDir, const Func &func) noexcept
{
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(rootDir)) {
        const fs::path& entryPath = entry.path();
        
        if (entry.is_directory()) {
            ForEachFileInSubDirectories(entryPath, func);
            continue;
        }
        
        if (func(entry)) {
            return entryPath;
        }
    }

    return std::nullopt;
}