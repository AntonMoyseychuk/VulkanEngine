#pragma once

#include "logger.h"


void AssertImpl(bool condition, Logger::Type loggerType, const char* file, const char* function, uint32_t line, const char* conditionStr, const char* message);


#if defined(AM_ASSERTION_ENABLED)
    #define AM_ASSERT(condition, formatt, ...)              AssertImpl(condition, Logger::Type::APPLICATION, __FILE__, __FUNCTION__, __LINE__, "condtion: " #condition, fmt::format(formatt, __VA_ARGS__).c_str())
    #define AM_ASSERT_GRAPHICS_API(condition, formatt, ...) AssertImpl(condition, Logger::Type::GRAPHICS_API, __FILE__, __FUNCTION__, __LINE__, "condtion: " #condition, fmt::format(formatt, __VA_ARGS__).c_str())
    #define AM_ASSERT_WINDOW(condition, formatt, ...)       AssertImpl(condition, Logger::Type::WINDOW_SYSTEM, __FILE__, __FUNCTION__, __LINE__, "condtion: " #condition, fmt::format(formatt, __VA_ARGS__).c_str())

    #define AM_ASSERT_FAIL(formatt, ...)              AM_ASSERT(false, formatt, __VA_ARGS__)
    #define AM_ASSERT_GRAPHICS_API_FAIL(formatt, ...) AM_ASSERT_GRAPHICS_API(false, formatt, __VA_ARGS__)
    #define AM_ASSERT_WINDOW_FAIL(formatt, ...)       AM_ASSERT_WINDOW(false, formatt, __VA_ARGS__)
#else
    #define AM_ASSERT(condition, formatt, ...)
    #define AM_ASSERT_GRAPHICS_API(condition, formatt, ...)
    #define AM_ASSERT_WINDOW(condition, formatt, ...)

    #define AM_ASSERT_FAIL(formatt, ...)
    #define AM_ASSERT_GRAPHICS_API_FAIL(formatt, ...)
    #define AM_ASSERT_WINDOW_FAIL(formatt, ...)
#endif