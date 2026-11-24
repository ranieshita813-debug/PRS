#include "PRS/Core/Log.h"
#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace PRS::Log
{
namespace
{
LogLevel g_minLevel = LogLevel::Trace;
std::mutex g_mutex;
const char* Name(LogLevel l)
{
    switch (l)
    {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Info:  return "INFO ";
    case LogLevel::Warn:  return "WARN ";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    }
    return "?";
}
} // namespace

void SetMinLevel(LogLevel level) { g_minLevel = level; }

void Write(LogLevel level, const char* file, int line, const char* fmt, ...)
{
    if (level < g_minLevel) return;
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    std::lock_guard<std::mutex> lock(g_mutex);
    FILE* out = (level >= LogLevel::Warn) ? stderr : stdout;
    if (level >= LogLevel::Error) std::fprintf(out, "[%s] %s (%s:%d)\n", Name(level), buffer, file, line);
    else                          std::fprintf(out, "[%s] %s\n", Name(level), buffer);
}
} // namespace PRS::Log
