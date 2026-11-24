#include "TestFramework.h"

int main()
{
    for (auto& t : Registry())
    {
        std::printf("[RUN ] %s\n", t.name);
        t.fn();
    }
    std::printf(Failures() ? "%d check(s) FAILED\n" : "All tests passed\n", Failures());
    return Failures() ? 1 : 0;
}
