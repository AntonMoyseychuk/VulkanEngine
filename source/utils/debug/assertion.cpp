#include "../../pch.h"

#include "assertion.h"


void AssertImpl(bool condition, int loggerType, const char* file, const char* function, uint32_t line, const char* conditionStr, const char* message)
{
    if (!condition) {
        const Logger::LoggerType type = static_cast<Logger::LoggerType>(loggerType);
        assert(type >= 0 && type < Logger::LoggerType_COUNT);

        Logger::Instance()->Error(type, false, file, function, line, conditionStr, message);
        AM_DEBUG_BREAK();
    }
}
