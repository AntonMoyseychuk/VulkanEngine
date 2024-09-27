#pragma once

#include "core.h"

#include <cstdint>


// Build type agnostic printing macro
#define COLOR_CODE_RESET "\033[0m"
#define COLOR_CODE_WHITE "\033[37m"
#define COLOR_CODE_YELLOW "\033[33m"
#define COLOR_CODE_RED "\033[31m"

#define AM_PRINT_FORMATED_COLORED_TEXT(colorCode, ostream, format, ...) \
    do { \
        fputs(colorCode, ostream); \
        fprintf_s(ostream, format, __VA_ARGS__); \
        fputs(COLOR_CODE_RESET, ostream); \
    } while(0)

#define AM_INFO_MSG(format, ...)    AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stdout, format, __VA_ARGS__)
#define AM_WARNING_MSG(format, ...) AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_YELLOW, stderr, format, __VA_ARGS__)
#define AM_ERROR_MSG(format, ...)   AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_RED, stdout, format, __VA_ARGS__)


// Debug only logging macro
template <typename... Args>
void LogError(const char* file, const char* function, uint64_t line, const char* format, Args&&... args) noexcept;

template <typename... Args>
void LogWarning(const char* file, const char* function, uint64_t line, const char* format, Args&&... args) noexcept;

template <typename... Args>
void LogInfo(const char* file, const char* function, uint64_t line, const char* format, Args&&... args) noexcept;

#if defined(AM_LOGGING_ENABLED)
    #define AM_LOG_ERROR(format, ...)   LogError(__FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
    #define AM_LOG_WARNING(format, ...) LogWarning(__FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
    #define AM_LOG_INFO(format, ...)    LogInfo(__FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#else
    #define AM_LOG_ERROR(format, ...)
    #define AM_LOG_WARNING(format, ...)
    #define AM_LOG_INFO(format, ...)
#endif

#include "logging.hpp"