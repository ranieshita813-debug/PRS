#include "TestFramework.h"
#include "PRS/Core/Memory/LinearAllocator.h"
#include "PRS/Core/Memory/PoolAllocator.h"

using namespace PRS;

TEST(LinearAllocator_AllocatesAndResets)
{
    LinearAllocator a(1024);
    void* p1 = a.Allocate(100);
    void* p2 = a.Allocate(100);
    CHECK(p1 != nullptr);
    CHECK(p2 != nullptr);
    CHECK(p1 != p2);
    CHECK(a.Used() >= 200);
    a.Reset();
    CHECK(a.Used() == 0);
}

TEST(LinearAllocator_ReturnsNullWhenFull)
{
    LinearAllocator a(64);
    CHECK(a.Allocate(64) != nullptr);
    CHECK(a.Allocate(1) == nullptr);
}

TEST(PoolAllocator_ReusesFreedSlots)
{
    PoolAllocator<int> pool(2);
    int* a = pool.Create(1);
    int* b = pool.Create(2);
    CHECK(pool.Create(3) == nullptr);
    CHECK(*a == 1 && *b == 2);
    pool.Destroy(a);
    int* c = pool.Create(9);
    CHECK(c == a);
    CHECK(pool.Used() == 2);
}
