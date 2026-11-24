#pragma once
#include "PRS/Core/Types.h"

namespace PRS
{
enum class LogLevel : u8 { Trace, Info, Warn, Error, Fatal };

namespace Log
{
void SetMinLevel(LogLevel level);
void Write(LogLevel level, const char* file, int line, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 4, 5)))
#endif
    ;
} // namespace Log
} // namespace PRS

#define PRS_TRACE(...) ::PRS::Log::Write(::PRS::LogLevel::Trace, __FILE__, __LINE__, __VA_ARGS__)
#define PRS_INFO(...)  ::PRS::Log::Write(::PRS::LogLevel::Info,  __FILE__, __LINE__, __VA_ARGS__)
#define PRS_WARN(...)  ::PRS::Log::Write(::PRS::LogLevel::Warn,  __FILE__, __LINE__, __VA_ARGS__)
#define PRS_ERROR(...) ::PRS::Log::Write(::PRS::LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)
#define PRS_FATAL(...) ::PRS::Log::Write(::PRS::LogLevel::Fatal, __FILE__, __LINE__, __VA_ARGS__)
