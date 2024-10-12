template <typename Func, typename... Args>
inline void ForEachFileInDirectory(const fs::path &dir, const Func &func, Args &&...args) noexcept
{
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_directory()) {
            ForEachFileInDirectory(entry.path(), func, std::forward<Args>(args)...);
            continue;
        }
        
        func(entry, std::forward<Args>(args)...);
    }
}