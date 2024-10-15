#include "pch.h"

#include "logger.h"

#include "config.h"
#include "utils/json/json.h"


static constexpr const char* JSON_LOGGER_CONFIG_LOGGERS_FIELD_NAME = "loggers";
static constexpr const char* JSON_LOGGER_CONFIG_LOGGER_NAME_FIELD_NAME = "name";
static constexpr const char* JSON_LOGGER_CONFIG_LOGGER_OUTPUT_PATTERN_FIELD_NAME = "pattern";


Logger::LoggerSystemInitInfo Logger::ParseLoggerSysInitInfoJson(const fs::path &pathToJson) noexcept
{
    std::optional<nlohmann::json> jsonOpt = amjson::ParseJson(pathToJson);
    if (!jsonOpt.has_value()) {
        return {};
    }

    const nlohmann::json& json = jsonOpt.value();
    
    const nlohmann::json& loggers = AM_GET_JSON_SUB_NODE(json, JSON_LOGGER_CONFIG_LOGGERS_FIELD_NAME);
    if (loggers.empty()) {
        AM_LOG_WARN("No custom loggers are specified in the log system configuration json file");
        return {};
    }

    LoggerSystemInitInfo info = {};
    info.infos.reserve(loggers.size());
    
    LoggerSystemInitInfo::LoggerInfo loggerInfo = {};
    for (const auto& logger : loggers.items()) {
        const nlohmann::json& value = logger.value();

        AM_GET_JSON_SUB_NODE(value, JSON_LOGGER_CONFIG_LOGGER_NAME_FIELD_NAME).get_to(loggerInfo.loggerName);
        AM_GET_JSON_SUB_NODE(value, JSON_LOGGER_CONFIG_LOGGER_OUTPUT_PATTERN_FIELD_NAME).get_to(loggerInfo.outputPattern);

        info.infos.emplace_back(loggerInfo);
    }

    return info;
}


std::shared_ptr<spdlog::logger> Logger::CreateDefaultSpdlogger() noexcept
{
    if (s_pDefaultLogger) {
        return s_pDefaultLogger;
    }

    static const std::string DEFAULT_LOGGER_NAME = "DEFAULT";
    static const std::string DEFAULT_LOGGER_PATTERN = "[%^%l%$] [%n]: %v";

    std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_st(DEFAULT_LOGGER_NAME);
    logger->set_pattern(DEFAULT_LOGGER_PATTERN);

    return logger;
}


Logger* Logger::Instance() noexcept
{
    return s_pInst.get();
}

std::shared_ptr<spdlog::logger> Logger::GetDefaultLogger() noexcept
{
    return s_pDefaultLogger;
}


bool Logger::Init()
{
    if (IsInitialized()) {
        AM_LOG_WARN("Log system is already initialized");
        return true;
    }

    LoggerSystemInitInfo initInfo = ParseLoggerSysInitInfoJson(paths::AM_LOGGER_CONFIG_FILE_PATH);

    s_pInst = std::unique_ptr<Logger>(new Logger(initInfo));

    return s_pInst != nullptr;
}


void Logger::Terminate() noexcept
{
    s_pInst = nullptr;
}


bool Logger::IsInitialized() noexcept
{
    return s_pInst != nullptr;
}


Logger::Logger(const LoggerSystemInitInfo &initInfo)
{
    if (initInfo.infos.empty()) {
        return;
    }

    m_loggers.reserve(initInfo.infos.size());
    for (const LoggerSystemInitInfo::LoggerInfo& info : initInfo.infos) {
        std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_st(info.loggerName);
        logger->set_pattern(info.outputPattern);

        m_loggers.emplace_back(logger);
    }
}


bool Logger::IsCustomLogger(LoggerType type) const noexcept
{
    return type < m_loggers.size() && m_loggers[type] != nullptr;
}


void amInitLogSystem() noexcept
{
#if defined(AM_LOGGING_ENABLED)
    if (!Logger::Init()) {
        AM_LOG_WARN("Unexpected problems occurred during the initialization of the log system. Using default logger.\n");
    }
#endif
}


void amTerminateLogSystem() noexcept
{
#if defined(AM_LOGGING_ENABLED)
    Logger::Terminate();
#endif
}