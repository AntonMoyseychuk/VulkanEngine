#include "logging.h"

template <typename... Args>
inline void Logger::Info(LoggerType type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char *format, Args &&...args)
{
    const auto& pLogger = IsLoggerInitialized() ? m_loggers[type] : s_defaultLogger;

    const std::string message = fmt::format(format, std::forward<Args>(args)...);
    
    if (printMessageOnly) {
        pLogger->info(message);
    } else {
        const std::string format0 = fmt::format("{}\n\t\t- File: {}\n\t\t- Function: {} ({})", message, file, function, line);
        
        pLogger->info(format0);
    }
}


template <typename... Args>
inline void Logger::Warn(LoggerType type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char *format, Args &&...args)
{
    const auto& pLogger = IsLoggerInitialized() ? m_loggers[type] : s_defaultLogger;

    const std::string message = fmt::format(format, std::forward<Args>(args)...);
    
    if (printMessageOnly) {
        pLogger->warn(message);
    } else {
        const std::string format0 = fmt::format("{}\n\t\t- File: {}\n\t\t- Function: {} ({})", message, file, function, line);
        
        pLogger->warn(format0);
    }
}


template <typename... Args>
inline void Logger::Error(LoggerType type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char *format, Args &&...args)
{
    const auto& pLogger = IsLoggerInitialized() ? m_loggers[type] : s_defaultLogger;

    const std::string message = fmt::format(format, std::forward<Args>(args)...);
    
    if (printMessageOnly) {
        pLogger->error(message);
    } else {
        const std::string format0 = fmt::format("{}\n\t\t- File: {}\n\t\t- Function: {} ({})", message, file, function, line);
        
        pLogger->error(format0);
    }
}