#pragma once
#if defined(__ANDROID__)
  #define PRS_PLATFORM_ANDROID 1
#elif defined(_WIN32)
  #define PRS_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
  #define PRS_PLATFORM_MACOS 1
#elif defined(__linux__)
  #define PRS_PLATFORM_LINUX 1
#endif

#if defined(_MSC_VER)
  #define PRS_DEBUG_BREAK() __debugbreak()
  #define PRS_FORCE_INLINE __forceinline
#else
  #define PRS_DEBUG_BREAK() __builtin_trap()
  #define PRS_FORCE_INLINE inline __attribute__((always_inline))
#endif

#if !defined(NDEBUG)
  #define PRS_DEBUG 1
#endif
