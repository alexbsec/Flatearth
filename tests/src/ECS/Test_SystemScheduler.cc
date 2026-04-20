#include <ECS/Scheduler/SystemScheduler.hpp>
#include <ECS/Registry.hpp>
#include <Core/FeMemory.hpp>

#include <vector>
#include <string_view>
#include <functional>

// ── tiny test framework (same runner as TestRunner.cc) ───────────────────────

void RegisterTest(std::string_view name, std::function<bool()> fn);

#define ASSERT(expr)  do { if (!(expr)) { return false; } } while(0)
#define PASS          return true

// ── stub systems ─────────────────────────────────────────────────────────────

using namespace flatearth;
using namespace flatearth::ecs;
using namespace flatearth::memory;

static std::vector<int> gOrder;

struct SysA : ISystem {
    void Update(Registry &, float32) override { gOrder.push_back(0); }
};
struct SysB : ISystem {
    void Update(Registry &, float32) override { gOrder.push_back(1); }
};
struct SysC : ISystem {
    void Update(Registry &, float32) override { gOrder.push_back(2); }
};

// ── helpers ──────────────────────────────────────────────────────────────────

static int indexOf(int val) {
    for (int i = 0; i < (int)gOrder.size(); ++i)
        if (gOrder[i] == val) return i;
    return -1;
}

// ── tests ────────────────────────────────────────────────────────────────────

// Build succeeds with no systems registered.
static bool test_build_empty() {
    MemoryManager mem;
    SystemScheduler sched(mem);
    auto res = sched.Build();
    ASSERT(res.has_value());
    PASS;
}

// Register + Build populates the sorted list; Update calls every system once.
static bool test_register_and_update() {
    MemoryManager mem;
    Registry reg(mem);
    SystemScheduler sched(mem);

    auto r = sched.Register<SysA>();
    ASSERT(r.has_value());

    auto res = sched.Build();
    ASSERT(res.has_value());

    gOrder.clear();
    sched.Update(reg, 0.016f);
    ASSERT(gOrder.size() == 1);
    ASSERT(gOrder[0] == 0);
    PASS;
}

// Ordering: SysA.Before<SysB>() → A must run before B.
static bool test_before_ordering() {
    MemoryManager mem;
    Registry reg(mem);
    SystemScheduler sched(mem);

    auto rA = sched.Register<SysA>();
    ASSERT(rA.has_value());
    rA.value().Before<SysB>();

    auto rB = sched.Register<SysB>();
    ASSERT(rB.has_value());

    ASSERT(sched.Build().has_value());

    gOrder.clear();
    sched.Update(reg, 0.016f);

    ASSERT(gOrder.size() == 2);
    ASSERT(indexOf(0) < indexOf(1)); // SysA(0) before SysB(1)
    PASS;
}

// Ordering: SysB.After<SysA>() → A must run before B.
static bool test_after_ordering() {
    MemoryManager mem;
    Registry reg(mem);
    SystemScheduler sched(mem);

    auto rA = sched.Register<SysA>();
    ASSERT(rA.has_value());

    auto rB = sched.Register<SysB>();
    ASSERT(rB.has_value());
    rB.value().After<SysA>();

    ASSERT(sched.Build().has_value());

    gOrder.clear();
    sched.Update(reg, 0.016f);

    ASSERT(gOrder.size() == 2);
    ASSERT(indexOf(0) < indexOf(1));
    PASS;
}

// Chain: A → B → C (A before B, B before C).
static bool test_chain_ordering() {
    MemoryManager mem;
    Registry reg(mem);
    SystemScheduler sched(mem);

    sched.Register<SysA>().value().Before<SysB>();
    sched.Register<SysB>().value().Before<SysC>();
    sched.Register<SysC>();

    ASSERT(sched.Build().has_value());

    gOrder.clear();
    sched.Update(reg, 0.016f);

    ASSERT(gOrder.size() == 3);
    ASSERT(indexOf(0) < indexOf(1));
    ASSERT(indexOf(1) < indexOf(2));
    PASS;
}

// Cycle A → B → A must be detected and Build() must fail.
static bool test_cycle_detection() {
    MemoryManager mem;
    SystemScheduler sched(mem);

    sched.Register<SysA>().value().Before<SysB>();
    sched.Register<SysB>().value().Before<SysA>();

    auto res = sched.Build();
    ASSERT(!res.has_value()); // expect failure
    PASS;
}

// Registering after Build() must fail.
static bool test_register_after_build_fails() {
    MemoryManager mem;
    SystemScheduler sched(mem);

    ASSERT(sched.Build().has_value());

    auto res = sched.Register<SysA>();
    ASSERT(!res.has_value());
    PASS;
}

// Unregister before Build removes the system; Update must not call it.
static bool test_unregister_before_build() {
    MemoryManager mem;
    Registry reg(mem);
    SystemScheduler sched(mem);

    sched.Register<SysA>();
    sched.Register<SysB>();
    sched.Unregister<SysA>();

    ASSERT(sched.Build().has_value());

    gOrder.clear();
    sched.Update(reg, 0.016f);

    ASSERT(gOrder.size() == 1);
    ASSERT(gOrder[0] == 1); // only SysB
    PASS;
}

// Reset() + re-Build runs systems again.
static bool test_reset_and_rebuild() {
    MemoryManager mem;
    Registry reg(mem);
    SystemScheduler sched(mem);

    sched.Register<SysA>();
    ASSERT(sched.Build().has_value());

    sched.Reset();
    ASSERT(sched.Build().has_value());

    gOrder.clear();
    sched.Update(reg, 0.016f);
    ASSERT(gOrder.size() == 1);
    PASS;
}

// Update before Build() is a no-op (does not crash, order stays empty).
static bool test_update_before_build_noop() {
    MemoryManager mem;
    Registry reg(mem);
    SystemScheduler sched(mem);

    sched.Register<SysA>();

    gOrder.clear();
    sched.Update(reg, 0.016f);
    ASSERT(gOrder.empty());
    PASS;
}

// ── registration ─────────────────────────────────────────────────────────────

namespace {
struct Registrar {
    Registrar() {
        RegisterTest("scheduler: build empty",              test_build_empty);
        RegisterTest("scheduler: register and update",      test_register_and_update);
        RegisterTest("scheduler: Before<> ordering",        test_before_ordering);
        RegisterTest("scheduler: After<> ordering",         test_after_ordering);
        RegisterTest("scheduler: chain A→B→C ordering",     test_chain_ordering);
        RegisterTest("scheduler: cycle detection",          test_cycle_detection);
        RegisterTest("scheduler: register after build fails", test_register_after_build_fails);
        RegisterTest("scheduler: unregister before build",  test_unregister_before_build);
        RegisterTest("scheduler: reset and rebuild",        test_reset_and_rebuild);
        RegisterTest("scheduler: update before build noop", test_update_before_build_noop);
    }
} gRegistrar;
}
