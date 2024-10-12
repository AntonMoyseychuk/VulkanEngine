#include "file.h"


template <typename Func, typename... Args>
inline void ForEachDirectory(const fs::path &root, const Func &func, Args &&...args) noexcept
{
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_directory()) {
            ForEachDirectory(entry.path(), func, std::forward<Args>(args)...);
            func(entry, std::forward<Args>(args)...);
        }
    }
}


template <typename Func, typename... Args>
inline void ForEachFileInSubDirectories(const fs::path& rootDir, const Func &func, Args &&...args) noexcept
{
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(rootDir)) {
        if (entry.is_directory()) {
            ForEachFileInSubDirectories(entry.path(), func, std::forward<Args>(args)...);
            continue;
        }
        
        func(entry, std::forward<Args>(args)...);
    }
}