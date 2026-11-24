#pragma once
#include "PRS/Core/Types.h"
#include <chrono>

namespace PRS
{
class Clock
{
public:
    Clock() : m_last(std::chrono::steady_clock::now()) {}
    // Returns seconds since the previous call.
    f32 Tick()
    {
        auto now = std::chrono::steady_clock::now();
        f32 dt = std::chrono::duration<f32>(now - m_last).count();
        m_last = now;
        return dt;
    }
private:
    std::chrono::steady_clock::time_point m_last;
};
} // namespace PRS
