#include "PRS/Jobs/JobSystem.h"
#include <algorithm>

namespace PRS
{
JobSystem::JobSystem(u32 workerCount)
{
    if (workerCount == 0)
    {
        u32 hw = std::thread::hardware_concurrency();
        workerCount = hw > 1 ? hw - 1 : 1; // leave one core for the main thread
    }
    for (u32 i = 0; i < workerCount; ++i) m_workers.emplace_back([this] { WorkerLoop(); });
}

JobSystem::~JobSystem()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_wake.notify_all();
    for (auto& t : m_workers) t.join();
}

void JobSystem::Submit(Job job)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(job));
        ++m_pending;
    }
    m_wake.notify_one();
}

void JobSystem::WaitIdle()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_idle.wait(lock, [this] { return m_pending.load() == 0; });
}

void JobSystem::ParallelFor(u32 count, const std::function<void(u32)>& fn, u32 chunk)
{
    for (u32 start = 0; start < count; start += chunk)
    {
        u32 end = std::min(start + chunk, count);
        Submit([&fn, start, end] { for (u32 i = start; i < end; ++i) fn(i); });
    }
    WaitIdle();
}

void JobSystem::WorkerLoop()
{
    for (;;)
    {
        Job job;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait(lock, [this] { return m_stop || !m_queue.empty(); });
            if (m_stop && m_queue.empty()) return;
            job = std::move(m_queue.front());
            m_queue.pop();
        }
        job();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (--m_pending == 0) m_idle.notify_all();
        }
    }
}
} // namespace PRS
