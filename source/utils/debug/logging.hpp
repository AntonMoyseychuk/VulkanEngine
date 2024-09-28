namespace utils
{
    template <typename... Args>
    inline void SpdlogPrint(std::shared_ptr<spdlog::logger> pLogger, spdlog::level::level_enum level, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args)
    {
        const std::string message = fmt::format(format, std::forward<Args>(args)...);
    
        if (printMessageOnly) {
            pLogger->log(level, message);
        } else {
            std::string format0 = fmt::format("{}\n", message);
            format0 += fmt::format("\t\t- File: {}\n", file);
            format0 += fmt::format("\t\t- Function: {} ({})", function, line);
            if (additionalInfo) {
                format0 += fmt::format("\n\t\t- Additional info: {}", additionalInfo);
            }
            
            pLogger->log(level, format0);
        }
    }   
}


template <typename... Args>
inline void Logger::Info(LoggerType type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args)
{
    const auto& pLogger = IsInitialized() && IsCustomLogger(type) ? m_loggers[type] : s_pDefaultLogger;

    utils::SpdlogPrint(pLogger, spdlog::level::level_enum::info, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
}


template <typename... Args>
inline void Logger::Warn(LoggerType type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args)
{
    const auto& pLogger = IsInitialized() && IsCustomLogger(type) ? m_loggers[type] : s_pDefaultLogger;

    utils::SpdlogPrint(pLogger, spdlog::level::level_enum::warn, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
}


template <typename... Args>
inline void Logger::Error(LoggerType type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args)
{
    const auto& pLogger = IsInitialized() && IsCustomLogger(type) ? m_loggers[type] : s_pDefaultLogger;

    utils::SpdlogPrint(pLogger, spdlog::level::level_enum::err, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
}


template <typename... Args>
inline void LoggerInfo(Logger::LoggerType type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args) noexcept
{
    if (Logger::IsInitialized()) {
        Logger::Instance()->Info(type, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
    } else {
        utils::SpdlogPrint(Logger::GetDefaultLogger(), spdlog::level::level_enum::info, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
    }
}


template <typename... Args>
inline void LoggerWarn(Logger::LoggerType type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args) noexcept
{
    if (Logger::IsInitialized()) {
        Logger::Instance()->Warn(type, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
    } else {
        utils::SpdlogPrint(Logger::GetDefaultLogger(), spdlog::level::level_enum::warn, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
    }
}


template <typename... Args>
inline void LoggerError(Logger::LoggerType type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args) noexcept
{
    if (Logger::IsInitialized()) {
        Logger::Instance()->Error(type, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
    } else {
        utils::SpdlogPrint(Logger::GetDefaultLogger(), spdlog::level::level_enum::err, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
    }
}