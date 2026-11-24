#pragma once
#include <cstdio>
#include <functional>
#include <vector>

struct TestCase { const char* name; std::function<void()> fn; };
inline std::vector<TestCase>& Registry() { static std::vector<TestCase> r; return r; }
inline int& Failures() { static int f = 0; return f; }
struct Registrar { Registrar(const char* n, std::function<void()> f) { Registry().push_back({n, std::move(f)}); } };

#define TEST(name) static void name(); static Registrar reg_##name(#name, name); static void name()
#define CHECK(cond) do { if (!(cond)) { std::printf("  FAILED: %s (%s:%d)\n", #cond, __FILE__, __LINE__); ++Failures(); } } while (0)
