#include <utility>

template <typename... Args>
inline void LogError(const char *file, const char *function, uint64_t line, const char *format, Args &&...args) noexcept
{
    AM_ERROR_MSG("[ERROR]: ");
    AM_ERROR_MSG(format, std::forward<Args>(args)...);

    AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stderr, "  [%s -> %s -> %u]\n", file, function, line);
}


template <typename... Args>
inline void LogWarning(const char *file, const char *function, uint64_t line, const char *format, Args &&...args) noexcept
{
    AM_WARNING_MSG("[WARNING]: ");
    AM_WARNING_MSG(format, std::forward<Args>(args)...);

    AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stdout, "  [%s -> %s -> %u]\n", file, function, line);
}


template <typename... Args>
inline void LogInfo(const char *file, const char *function, uint64_t line, const char *format, Args &&...args) noexcept
{
    AM_INFO_MSG("[INFO]: ");
    AM_INFO_MSG(format, std::forward<Args>(args)...);

    AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stdout, "  [%s -> %s -> %u]\n", file, function, line);
}