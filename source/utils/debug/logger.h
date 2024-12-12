#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdint>


#define AM_OUTPUT_COLOR_RESET_ASCII_CODE      "\033[0m"
#define AM_OUTPUT_COLOR_BLACK_ASCII_CODE      "\033[30m"
#define AM_OUTPUT_COLOR_RED_ASCII_CODE        "\033[31m"
#define AM_OUTPUT_COLOR_GREEN_ASCII_CODE      "\033[32m"
#define AM_OUTPUT_COLOR_YELLOW_ASCII_CODE     "\033[33m"
#define AM_OUTPUT_COLOR_BLUE_ASCII_CODE       "\033[34m"
#define AM_OUTPUT_COLOR_MAGENTA_ASCII_CODE    "\033[35m"
#define AM_OUTPUT_COLOR_CYAN_ASCII_CODE       "\033[36m"
#define AM_OUTPUT_COLOR_WHITE_ASCII_CODE      "\033[37m"

#define AM_MAKE_COLORED_TEXT(color, text) color text AM_OUTPUT_COLOR_RESET_ASCII_CODE


class Logger
{
public:
    enum class Type
    {
        APPLICATION,
        GRAPHICS_API,
        WINDOW_SYSTEM,
        COUNT,
    };

public:
    static Logger* Instance() noexcept;

    static std::shared_ptr<spdlog::logger> GetDefaultLogger() noexcept;

    static bool Init();
    static void Terminate() noexcept;

    static bool IsInitialized() noexcept;

    Logger(const Logger& app) = delete;
    Logger& operator=(const Logger& app) = delete;
    
    Logger(Logger&& app) = delete;
    Logger& operator=(Logger&& app) = delete;

    template <typename... Args>
    void Info(Type type, bool printMessageOnly, const char* file, const char* function, uint32_t line, const char* additionalInfo, const char* format, Args&&... args);

    template <typename... Args>
    void Warn(Type type, bool printMessageOnly, const char* file, const char* function, uint32_t line, const char* additionalInfo, const char* format, Args&&... args);

    template <typename... Args>
    void Error(Type type, bool printMessageOnly, const char* file, const char* function, uint32_t line, const char* additionalInfo, const char* format, Args&&... args);


private:
    static std::shared_ptr<spdlog::logger> CreateDefaultSpdlogger() noexcept;

private:
    Logger();

    bool IsCustomLogger(Type type) const noexcept;

private:
    static inline std::unique_ptr<Logger> s_pInst = nullptr;
    static inline std::shared_ptr<spdlog::logger> s_pDefaultLogger = CreateDefaultSpdlogger();

private:
    std::vector<std::shared_ptr<spdlog::logger>> m_loggers;
};


template <typename... Args>
inline void LoggerInfo(Logger::Type type, bool printMessageOnly, const char* file, const char* function, uint32_t line, const char* additionalInfo, const char* format, Args&&... args) noexcept;

template <typename... Args>
inline void LoggerWarn(Logger::Type type, bool printMessageOnly, const char* file, const char* function, uint32_t line, const char* additionalInfo, const char* format, Args&&... args) noexcept;

template <typename... Args>
inline void LoggerError(Logger::Type type, bool printMessageOnly, const char* file, const char* function, uint32_t line, const char* additionalInfo, const char* format, Args&&... args) noexcept;


#include "logger.hpp"


void amInitLogSystem() noexcept;
void amTerminateLogSystem() noexcept;

bool amIsLogSystemInitialized() noexcept;


#if defined(AM_LOGGING_ENABLED)
    #define AM_LOG_ERROR(format, ...) LoggerError(Logger::Type::APPLICATION, false, __FILE__, __FUNCTION__, __LINE__, nullptr, format, __VA_ARGS__)
    #define AM_LOG_WARN(format, ...)  LoggerWarn(Logger::Type::APPLICATION, true, __FILE__, __FUNCTION__, __LINE__, nullptr, format, __VA_ARGS__)
    #define AM_LOG_INFO(format, ...)  LoggerInfo(Logger::Type::APPLICATION, true, __FILE__, __FUNCTION__, __LINE__, nullptr, format, __VA_ARGS__)

    #define AM_LOG_WINDOW_ERROR(format, ...) LoggerError(Logger::Type::WINDOW_SYSTEM, false, __FILE__, __FUNCTION__, __LINE__, nullptr, format, __VA_ARGS__)
    #define AM_LOG_WINDOW_WARN(format, ...)  LoggerWarn(Logger::Type::WINDOW_SYSTEM, true, __FILE__, __FUNCTION__, __LINE__, nullptr, format, __VA_ARGS__)
    #define AM_LOG_WINDOW_INFO(format, ...)  LoggerInfo(Logger::Type::WINDOW_SYSTEM, true, __FILE__, __FUNCTION__, __LINE__, nullptr, format, __VA_ARGS__)

    #define AM_LOG_GRAPHICS_API_ERROR(format, ...) LoggerError(Logger::Type::GRAPHICS_API, false, __FILE__, __FUNCTION__, __LINE__, nullptr, format, __VA_ARGS__)
    #define AM_LOG_GRAPHICS_API_WARN(format, ...)  LoggerWarn(Logger::Type::GRAPHICS_API, true, __FILE__, __FUNCTION__, __LINE__, nullptr, format, __VA_ARGS__)
    #define AM_LOG_GRAPHICS_API_INFO(format, ...)  LoggerInfo(Logger::Type::GRAPHICS_API, true, __FILE__, __FUNCTION__, __LINE__, nullptr, format, __VA_ARGS__)
#else
    #define AM_LOG_ERROR(format, ...)
    #define AM_LOG_WARN(format, ...)
    #define AM_LOG_INFO(format, ...)

    #define AM_LOG_WINDOW_ERROR(format, ...)
    #define AM_LOG_WINDOW_WARN(format, ...)
    #define AM_LOG_WINDOW_INFO(format, ...)

    #define AM_LOG_GRAPHICS_API_ERROR(format, ...)
    #define AM_LOG_GRAPHICS_API_WARN(format, ...)
    #define AM_LOG_GRAPHICS_API_INFO(format, ...)
#endif