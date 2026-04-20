#include <ECS/Registry.hpp>
#include <Core/FeMemory.hpp>

#include <string_view>
#include <functional>

void RegisterTest(std::string_view name, std::function<bool()> fn);

using namespace flatearth;
using namespace flatearth::ecs;
using namespace flatearth::memory;

#define ASSERT(expr) do { if (!(expr)) return false; } while(0)
#define PASS return true

struct Pos { float x, y; };
struct Vel { float dx, dy; };
struct Marker { int id; };

static bool test_single_component_view_iterates_all() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId a = reg.Create(); reg.Insert<Pos>(a, {1.f, 0.f});
    EntityId b = reg.Create(); reg.Insert<Pos>(b, {2.f, 0.f});
    EntityId c = reg.Create(); reg.Insert<Pos>(c, {3.f, 0.f});

    int count = 0;
    for (auto [id, p] : reg.ViewOf<Pos>()) {
        (void)id; (void)p;
        ++count;
    }
    ASSERT(count == 3);
    PASS;
}

static bool test_multi_component_view_skips_missing() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId a = reg.Create();
    EntityId b = reg.Create();
    EntityId c = reg.Create();

    reg.Insert<Pos>(a, {1.f, 0.f});
    reg.Insert<Vel>(a, {0.f, 1.f});

    reg.Insert<Pos>(b, {2.f, 0.f}); // no Vel → must be skipped

    reg.Insert<Pos>(c, {3.f, 0.f});
    reg.Insert<Vel>(c, {0.f, 2.f});

    int count = 0;
    for (auto [id, p, v] : reg.ViewOf<Pos, Vel>()) {
        (void)p; (void)v;
        ASSERT(id == a || id == c);
        ++count;
    }
    ASSERT(count == 2);
    PASS;
}

static bool test_view_empty_when_no_components() {
    MemoryManager mem;
    Registry reg(mem);
    reg.Create();
    reg.Create();

    int count = 0;
    for (auto [id, p] : reg.ViewOf<Pos>()) {
        (void)id; (void)p;
        ++count;
    }
    ASSERT(count == 0);
    PASS;
}

static bool test_view_reflects_remove() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId a = reg.Create(); reg.Insert<Pos>(a, {1.f, 0.f});
    EntityId b = reg.Create(); reg.Insert<Pos>(b, {2.f, 0.f});
    reg.Remove<Pos>(a);

    int count = 0;
    for (auto [id, p] : reg.ViewOf<Pos>()) {
        ASSERT(id == b);
        (void)p;
        ++count;
    }
    ASSERT(count == 1);
    PASS;
}

static bool test_view_mutation_persists() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId e = reg.Create();
    reg.Insert<Pos>(e, {0.f, 0.f});

    for (auto [id, p] : reg.ViewOf<Pos>())
        p.x = 99.f;

    ASSERT(reg.Get<Pos>(e).x == 99.f);
    PASS;
}

static bool test_three_component_view() {
    MemoryManager mem;
    Registry reg(mem);

    EntityId full = reg.Create();
    reg.Insert<Pos>(full, {0.f, 0.f});
    reg.Insert<Vel>(full, {1.f, 1.f});
    reg.Insert<Marker>(full, {42});

    EntityId partial = reg.Create();
    reg.Insert<Pos>(partial, {0.f, 0.f});
    reg.Insert<Vel>(partial, {1.f, 1.f}); // no Marker

    int count = 0;
    for (auto [id, p, v, t] : reg.ViewOf<Pos, Vel, Marker>()) {
        ASSERT(id == full);
        ASSERT(t.id == 42);
        (void)p; (void)v;
        ++count;
    }
    ASSERT(count == 1);
    PASS;
}

namespace {
struct Reg {
    Reg() {
        RegisterTest("view: single component iterates all",    test_single_component_view_iterates_all);
        RegisterTest("view: multi component skips missing",    test_multi_component_view_skips_missing);
        RegisterTest("view: empty when no components",         test_view_empty_when_no_components);
        RegisterTest("view: reflects remove",                  test_view_reflects_remove);
        RegisterTest("view: mutation persists",                test_view_mutation_persists);
        RegisterTest("view: three component view",             test_three_component_view);
    }
} gReg;
}
