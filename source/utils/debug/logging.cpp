#include "../../pch.h"

#include "logging.h"


Logger::LoggerSystemInitInfo Logger::ParseLoggerSysInitInfoJson(const std::filesystem::path &pathToJson) noexcept
{
    std::optional<nlohmann::json> jsonOpt = ParseJson(pathToJson);
    if (!jsonOpt.has_value()) {
        return {};
    }

    const nlohmann::json& json = jsonOpt.value();
    
    const auto& loggers = json["loggers"];
    if (loggers.empty()) {
        return {};
    }

    LoggerSystemInitInfo info = {};
    info.infos.reserve(loggers.size());
    
    LoggerSystemInitInfo::LoggerInfo loggerInfo = {};
    for (const auto& logger : loggers.items()) {
        const auto& value = logger.value();
        value["name"].get_to(loggerInfo.loggerName);
        value["pattern"].get_to(loggerInfo.outputPattern);

        info.infos.emplace_back(loggerInfo);
    }

    return info;
}


Logger* Logger::Instance() noexcept
{
    return s_pInst.get();
}


bool Logger::Init()
{
    if (IsLoggerInitialized()) {
        return true;
    }

    s_pDefaultLogger->set_pattern(DEFAULT_LOGGER_PATTERN);

    LoggerSystemInitInfo initInfo = ParseLoggerSysInitInfoJson(paths::AM_LOGGER_CONFIG_FILE_PATH);

    s_pInst = std::unique_ptr<Logger>(new Logger(initInfo));

    return s_pInst != nullptr;
}


void Logger::Terminate() noexcept
{
    s_pInst = nullptr;
    s_pDefaultLogger = nullptr;
}


bool Logger::IsLoggerInitialized() noexcept
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