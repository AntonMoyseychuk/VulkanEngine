#include "pch.h"

#include "assertion.h"

#include <cassert>


void AssertImpl(bool condition, Logger::Type loggerType, const char* file, const char* function, uint32_t line, const char* conditionStr, const char* message)
{
    if (!condition) {
        const size_t type = static_cast<size_t>(loggerType);
        assert(type >= 0 && type < static_cast<size_t>(Logger::Type::COUNT));

        LoggerError(loggerType, false, file, function, line, conditionStr, message);
        AM_DEBUG_BREAK();
    }
}
