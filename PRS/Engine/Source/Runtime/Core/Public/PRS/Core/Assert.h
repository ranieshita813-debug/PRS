#pragma once
#include "PRS/Core/Log.h"
#include "PRS/Core/Platform.h"

#if defined(PRS_DEBUG)
  #define PRS_ASSERT(cond, ...) \
      do { if (!(cond)) { PRS_FATAL("Assertion failed: %s | " __VA_ARGS__ "", #cond); PRS_DEBUG_BREAK(); } } while (0)
#else
  #define PRS_ASSERT(cond, ...) do { (void)sizeof(cond); } while (0)
#endif

// Always-on check, even in release.
#define PRS_VERIFY(cond, ...) \
    do { if (!(cond)) { PRS_FATAL("Verify failed: %s | " __VA_ARGS__ "", #cond); PRS_DEBUG_BREAK(); } } while (0)
