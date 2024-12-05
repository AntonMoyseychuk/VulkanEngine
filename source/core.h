#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #define AM_OS_WINDOWS
#else
  #error Currently, only Windows is supported
#endif


#if defined(AM_GRAPHICS_API_VULKAN)
  #if defined(AM_OS_WINDOWS)
    #define VK_USE_PLATFORM_WIN32_KHR
  #endif
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


#if defined(_MSC_VER) && !defined(__INLINING__)
  #define AM_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
  #if __cplusplus >= 201703L
    #define AM_FORCE_INLINE [[nodiscard]] __attribute__((always_inline))
  #else
    #define AM_FORCE_INLINE __attribute__((always_inline))
  #endif
#else
  #define AM_FORCE_INLINE inline
#endif


#if __cplusplus == 201703L
  // From C++17
  #define AM_MAYBE_UNUSED [[maybe_unused]]
#else
  #define AM_MAYBE_UNUSED
#endif