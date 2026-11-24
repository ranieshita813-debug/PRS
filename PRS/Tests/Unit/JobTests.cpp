#include "TestFramework.h"
#include "PRS/Jobs/JobSystem.h"
#include <atomic>

using namespace PRS;

TEST(JobSystem_RunsAllJobs)
{
    JobSystem js(4);
    std::atomic<int> counter{0};
    for (int i = 0; i < 1000; ++i) js.Submit([&] { ++counter; });
    js.WaitIdle();
    CHECK(counter == 1000);
}

TEST(JobSystem_ParallelFor)
{
    JobSystem js(4);
    std::vector<int> data(10000, 0);
    js.ParallelFor(static_cast<u32>(data.size()), [&](u32 i) { data[i] = static_cast<int>(i); });
    bool ok = true;
    for (size_t i = 0; i < data.size(); ++i) ok = ok && data[i] == static_cast<int>(i);
    CHECK(ok);
}
