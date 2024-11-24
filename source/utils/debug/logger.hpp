namespace utils
{
    template <typename... Args>
    inline void SpdlogPrint(std::shared_ptr<spdlog::logger> pLogger, spdlog::level::level_enum level, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args)
    {
        const std::string message = fmt::format(format, std::forward<Args>(args)...);
    
        if (printMessageOnly) {
            pLogger->log(level, message);
        } else {
            static constexpr const char* NOT_SPECIFIED_MSG = "Not specified";

            const char* resFile = file ? file : NOT_SPECIFIED_MSG;
            const char* resFunction = function ? function : NOT_SPECIFIED_MSG;
            const uint32_t resLine = function ? line : 0;
            const char* resAdditionalInfo = additionalInfo ? additionalInfo : NOT_SPECIFIED_MSG;

            std::string format0 = fmt::format("{}\n    - File: {} (line: {})\n    - Function: {}\n    - Additional info: {}",
                message, resFile, resLine, resFunction, resAdditionalInfo);
            
            pLogger->log(level, format0);
        }
    }   
}


template <typename... Args>
inline void Logger::Info(Type type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args)
{
    const auto& pLogger = IsInitialized() && IsCustomLogger(type) ? m_loggers[(size_t)type] : s_pDefaultLogger;

    utils::SpdlogPrint(pLogger, spdlog::level::level_enum::info, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
}


template <typename... Args>
inline void Logger::Warn(Type type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args)
{
    const auto& pLogger = IsInitialized() && IsCustomLogger(type) ? m_loggers[(size_t)type] : s_pDefaultLogger;

    utils::SpdlogPrint(pLogger, spdlog::level::level_enum::warn, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
}


template <typename... Args>
inline void Logger::Error(Type type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args)
{
    const auto& pLogger = IsInitialized() && IsCustomLogger(type) ? m_loggers[(size_t)type] : s_pDefaultLogger;

    utils::SpdlogPrint(pLogger, spdlog::level::level_enum::err, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
}


template <typename... Args>
inline void LoggerInfo(Logger::Type type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args) noexcept
{
    if (Logger::IsInitialized()) {
        Logger::Instance()->Info(type, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
    } else {
        utils::SpdlogPrint(Logger::GetDefaultLogger(), spdlog::level::level_enum::info, printMessageOnly, file, function, line, additionalInfo, format, std::forward<Args>(args)...);
    }
}


template <typename... Args>
inline void LoggerWarn(Logger::Type type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args) noexcept
{
    std::string message = fmt::format(format, std::forward<Args>(args)...);
    std::string coloredMessage = fmt::format("{}{}{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, message, AM_OUTPUT_COLOR_RESET_ASCII_CODE);

    if (Logger::IsInitialized()) {
        Logger::Instance()->Warn(type, printMessageOnly, file, function, line, additionalInfo, coloredMessage.c_str());
    } else {
        utils::SpdlogPrint(Logger::GetDefaultLogger(), spdlog::level::level_enum::warn, printMessageOnly, file, function, line, additionalInfo, coloredMessage.c_str());
    }
}


template <typename... Args>
inline void LoggerError(Logger::Type type, bool printMessageOnly, const char *file, const char *function, uint32_t line, const char* additionalInfo, const char *format, Args &&...args) noexcept
{
    std::string message = fmt::format(format, std::forward<Args>(args)...);

    if (Logger::IsInitialized()) {
        Logger::Instance()->Error(type, printMessageOnly, file, function, line, additionalInfo, "{}{}{}", AM_OUTPUT_COLOR_RED_ASCII_CODE, message, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
    } else {
        utils::SpdlogPrint(Logger::GetDefaultLogger(), spdlog::level::level_enum::err, printMessageOnly, file, function, line, additionalInfo, "{}{}{}", AM_OUTPUT_COLOR_RED_ASCII_CODE, message, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
    }
}