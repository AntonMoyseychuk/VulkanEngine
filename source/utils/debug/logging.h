#pragma once

#include "core.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdint>


class Logger
{
public:
    enum LoggerType
    {
        // NOTE: Don't change order of this enum, needs remake in the future
        LoggerType_COMMON,
        LoggerType_GRAPHICS_API,
        LoggerType_WINDOW_SYSTEM,
        LoggerType_COUNT,
    };

public:
    static Logger* Instance() noexcept;

    static bool Init();
    static void Terminate() noexcept;

    static bool IsLoggerInitialized() noexcept;

    Logger(const Logger& app) = delete;
    Logger& operator=(const Logger& app) = delete;
    
    Logger(Logger&& app) = delete;
    Logger& operator=(Logger&& app) = delete;

    template <typename... Args>
    void Info(LoggerType type, bool printMessageOnly, const char* file, const char* function, uint32_t line, const char* format, Args&&... args);

    template <typename... Args>
    void Warn(LoggerType type, bool printMessageOnly, const char* file, const char* function, uint32_t line, const char* format, Args&&... args);

    template <typename... Args>
    void Error(LoggerType type,  bool printMessageOnly, const char* file, const char* function, uint32_t line, const char* format, Args&&... args);


private:
    struct LoggerSystemInitInfo
    {
        struct LoggerInfo
        {
            std::string loggerName;
            std::string outputPattern;
        };

        std::vector<LoggerInfo> infos;
    };

    static LoggerSystemInitInfo ParseLoggerSysInitInfoJson(const std::filesystem::path& pathToJson) noexcept;

private:
    Logger(const LoggerSystemInitInfo& initInfo);

private:
    static inline const std::string DEFAULT_LOGGER_NAME = "default";
    static inline const std::string DEFAULT_LOGGER_PATTERN = "[%^%l%$] [%n]: %v";

    static inline std::unique_ptr<Logger> s_pInst = nullptr;
    static inline std::shared_ptr<spdlog::logger> s_pDefaultLogger = spdlog::stdout_color_st(DEFAULT_LOGGER_NAME);

private:
    std::vector<std::shared_ptr<spdlog::logger>> m_loggers;
};

#include "logging.hpp"


#if defined(AM_LOGGING_ENABLED)
    #define AM_LOG_ERROR(format, ...) Logger::Instance()->Error(Logger::LoggerType_COMMON, false, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
    #define AM_LOG_WARN(format, ...)  Logger::Instance()->Warn(Logger::LoggerType_COMMON, true, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
    #define AM_LOG_INFO(format, ...)  Logger::Instance()->Info(Logger::LoggerType_COMMON, true, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)

    #define AM_LOG_WINDOW_ERROR(format, ...) Logger::Instance()->Error(Logger::LoggerType_WINDOW_SYSTEM, false, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
    #define AM_LOG_WINDOW_WARN(format, ...)  Logger::Instance()->Warn(Logger::LoggerType_WINDOW_SYSTEM, true, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
    #define AM_LOG_WINDOW_INFO(format, ...)  Logger::Instance()->Info(Logger::LoggerType_WINDOW_SYSTEM, true, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)

    #define AM_LOG_GFX_API_ERROR(format, ...) Logger::Instance()->Error(Logger::LoggerType_GRAPHICS_API, false, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
    #define AM_LOG_GFX_API_WARN(format, ...)  Logger::Instance()->Warn(Logger::LoggerType_GRAPHICS_API, true, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
    #define AM_LOG_GFX_API_INFO(format, ...)  Logger::Instance()->Info(Logger::LoggerType_GRAPHICS_API, true, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#else
    #define AM_LOG_ERROR(format, ...)
    #define AM_LOG_WARN(format, ...)
    #define AM_LOG_INFO(format, ...)

    #define AM_LOG_WINDOW_ERROR(format, ...)
    #define AM_LOG_WINDOW_WARN(format, ...)
    #define AM_LOG_WINDOW_INFO(format, ...)

    #define AM_LOG_GFX_API_ERROR(format, ...)
    #define AM_LOG_GFX_API_WARN(format, ...)
    #define AM_LOG_GFX_API_INFO(format, ...)
#endif