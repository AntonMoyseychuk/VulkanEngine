#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #define AM_OS_WINDOWS
#else
  #error Currently, only Windows is supported
#endif


#if !defined(AM_GRAPHICS_API_VULKAN)
  #error Currently, only Vulkan is supported
#endif


#if defined(NDEBUG)
  #define AM_RELEASE
#else
  #define AM_DEBUG
#endif


#if defined(AM_DEBUG)
  #define AM_ASSERTION_ENABLED
#endif

#if defined(AM_DEBUG)
  #define AM_LOGGING_ENABLED
#endif

#if defined(AM_DEBUG)
  #define AM_VK_VALIDATION_LAYERS_ENABLED
#endif


#if defined(_MSC_VER)
  #define AM_DEBUG_BREAK() __debugbreak()
#elif defined(__clang__)
  #define AM_DEBUG_BREAK() __builtin_trap()
#else
  #error Currently, only MSVC and Clang are supported
#endif