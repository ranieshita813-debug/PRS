#pragma once
#include "PRS/Core/Types.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace PRS
{
// Basic thread-pool job system. Upgrade path: work-stealing queues / fibers (see Docs/Roadmap.md).
class JobSystem
{
public:
    using Job = std::function<void()>;

    explicit JobSystem(u32 workerCount = 0);
    ~JobSystem();
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    void Submit(Job job);
    void WaitIdle(); // blocks until all submitted jobs have finished
    u32 WorkerCount() const { return static_cast<u32>(m_workers.size()); }

    // Splits [0,count) into chunks and runs fn(index) across workers, then waits.
    void ParallelFor(u32 count, const std::function<void(u32)>& fn, u32 chunk = 64);

private:
    void WorkerLoop();

    std::vector<std::thread> m_workers;
    std::queue<Job> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::condition_variable m_idle;
    std::atomic<u32> m_pending{0};
    bool m_stop = false;
};
} // namespace PRS
