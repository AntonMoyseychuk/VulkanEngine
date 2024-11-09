template <typename Func>
inline void ForEachDirectory(const fs::path& rootDir, const Func& func, uint32_t dirTreeDepth) noexcept
{
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(rootDir)) {
        if (entry.is_directory()) {
            func(entry);

            if (dirTreeDepth != 0) {
                ForEachDirectory(entry.path(), func, dirTreeDepth - 1);
            }
        }
    }
}


template <typename Func>
inline void ForEachFile(const fs::path &rootDir, const Func &func, uint32_t dirTreeDepth) noexcept
{
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(rootDir)) {
        if (entry.is_directory() && (dirTreeDepth != 0)) {
            ForEachFile(entry.path(), func, dirTreeDepth - 1);
            continue;
        }
        
        func(entry);
    }
}


template <typename Func>
inline std::optional<fs::path> FindFirstFileIf(const fs::path &rootDir, const Func &func, uint32_t dirTreeDepth) noexcept
{
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(rootDir)) {
        if (!entry.is_directory() && func(entry)) {
            return entry.path();
        } else if (entry.is_directory() && (dirTreeDepth != 0)) {
            return FindFirstFileIf(entry.path(), func, dirTreeDepth - 1);
        }
    }

    return std::nullopt;
}