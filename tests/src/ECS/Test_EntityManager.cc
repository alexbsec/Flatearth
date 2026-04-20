#include <ECS/EntityManager.hpp>
#include <ECS/ECSTypes.hpp>
#include <Core/FeMemory.hpp>

#include <string_view>
#include <functional>

void RegisterTest(std::string_view name, std::function<bool()> fn);

using namespace flatearth;
using namespace flatearth::ecs;
using namespace flatearth::memory;

#define ASSERT(expr) do { if (!(expr)) return false; } while(0)
#define PASS return true

static bool test_create_is_alive() {
    MemoryManager mem;
    EntityManager em(mem);
    EntityId id = em.Create();
    ASSERT(id != 0);
    ASSERT(em.IsAlive(id));
    ASSERT(em.AliveCount() == 1);
    PASS;
}

static bool test_destroy_not_alive() {
    MemoryManager mem;
    EntityManager em(mem);
    EntityId id = em.Create();
    em.Destroy(id);
    ASSERT(!em.IsAlive(id));
    ASSERT(em.AliveCount() == 0);
    PASS;
}

static bool test_id_reuse_after_destroy() {
    MemoryManager mem;
    EntityManager em(mem);
    EntityId a = em.Create();
    em.Destroy(a);
    EntityId b = em.Create();
    // same slot reused but generation bumped — IDs must differ
    ASSERT(IdIndex(a) == IdIndex(b));
    ASSERT(IdGeneration(b) == IdGeneration(a) + 1);
    ASSERT(a != b);
    PASS;
}

static bool test_stale_id_not_alive() {
    MemoryManager mem;
    EntityManager em(mem);
    EntityId a = em.Create();
    em.Destroy(a);
    EntityId b = em.Create(); // reuses slot
    ASSERT(!em.IsAlive(a));   // old id is stale
    ASSERT(em.IsAlive(b));
    PASS;
}

static bool test_null_id_not_alive() {
    MemoryManager mem;
    EntityManager em(mem);
    ASSERT(!em.IsAlive(0));
    PASS;
}

static bool test_multiple_entities_alive_count() {
    MemoryManager mem;
    EntityManager em(mem);
    EntityId a = em.Create();
    EntityId b = em.Create();
    EntityId c = em.Create();
    ASSERT(em.AliveCount() == 3);
    em.Destroy(b);
    ASSERT(em.AliveCount() == 2);
    ASSERT(em.IsAlive(a));
    ASSERT(!em.IsAlive(b));
    ASSERT(em.IsAlive(c));
    PASS;
}

static bool test_destroy_stale_is_noop() {
    MemoryManager mem;
    EntityManager em(mem);
    EntityId a = em.Create();
    em.Destroy(a);
    uint32 count = em.AliveCount();
    em.Destroy(a); // should be ignored
    ASSERT(em.AliveCount() == count);
    PASS;
}

namespace {
struct Reg {
    Reg() {
        RegisterTest("entity manager: create is alive",          test_create_is_alive);
        RegisterTest("entity manager: destroy not alive",        test_destroy_not_alive);
        RegisterTest("entity manager: id reuse after destroy",   test_id_reuse_after_destroy);
        RegisterTest("entity manager: stale id not alive",       test_stale_id_not_alive);
        RegisterTest("entity manager: null id not alive",        test_null_id_not_alive);
        RegisterTest("entity manager: multiple alive count",     test_multiple_entities_alive_count);
        RegisterTest("entity manager: destroy stale is noop",    test_destroy_stale_is_noop);
    }
} gReg;
}
