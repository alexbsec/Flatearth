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

struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health   { int hp; };

static bool test_create_and_destroy() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId a = reg.Create();
    EntityId b = reg.Create();
    ASSERT(a != 0 && b != 0 && a != b);
    reg.Destroy(a);
    PASS;
}

static bool test_insert_and_get() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId e = reg.Create();
    reg.Insert<Position>(e, {1.0f, 2.0f});
    auto& p = reg.Get<Position>(e);
    ASSERT(p.x == 1.0f && p.y == 2.0f);
    PASS;
}

static bool test_has_component() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId e = reg.Create();
    ASSERT(!reg.Has<Position>(e));
    reg.Insert<Position>(e, {0.f, 0.f});
    ASSERT(reg.Has<Position>(e));
    PASS;
}

static bool test_remove_component() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId e = reg.Create();
    reg.Insert<Position>(e, {3.f, 4.f});
    reg.Remove<Position>(e);
    ASSERT(!reg.Has<Position>(e));
    PASS;
}

static bool test_components_isolated_between_entities() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId a = reg.Create();
    EntityId b = reg.Create();
    reg.Insert<Position>(a, {1.f, 0.f});
    reg.Insert<Position>(b, {2.f, 0.f});
    ASSERT(reg.Get<Position>(a).x == 1.f);
    ASSERT(reg.Get<Position>(b).x == 2.f);
    PASS;
}

static bool test_multiple_component_types() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId e = reg.Create();
    reg.Insert<Position>(e, {5.f, 6.f});
    reg.Insert<Health>(e,   {100});
    ASSERT(reg.Has<Position>(e));
    ASSERT(reg.Has<Health>(e));
    ASSERT(!reg.Has<Velocity>(e));
    ASSERT(reg.Get<Health>(e).hp == 100);
    PASS;
}

static bool test_insert_on_dead_entity_ignored() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId e = reg.Create();
    reg.Destroy(e);
    reg.Insert<Position>(e, {9.f, 9.f}); // should be silently ignored
    ASSERT(!reg.Has<Position>(e));
    PASS;
}

static bool test_mutate_via_get() {
    MemoryManager mem;
    Registry reg(mem);
    EntityId e = reg.Create();
    reg.Insert<Position>(e, {0.f, 0.f});
    reg.Get<Position>(e).x = 42.f;
    ASSERT(reg.Get<Position>(e).x == 42.f);
    PASS;
}

namespace {
struct Reg {
    Reg() {
        RegisterTest("registry: create and destroy",                test_create_and_destroy);
        RegisterTest("registry: insert and get",                    test_insert_and_get);
        RegisterTest("registry: has component",                     test_has_component);
        RegisterTest("registry: remove component",                  test_remove_component);
        RegisterTest("registry: components isolated between entities", test_components_isolated_between_entities);
        RegisterTest("registry: multiple component types",          test_multiple_component_types);
        RegisterTest("registry: insert on dead entity ignored",     test_insert_on_dead_entity_ignored);
        RegisterTest("registry: mutate via get",                    test_mutate_via_get);
    }
} gReg;
}
