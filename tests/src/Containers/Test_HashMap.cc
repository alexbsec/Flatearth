#include "Containers/DArray.hpp"
#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"

#include <string_view>
#include <functional>

void RegisterTest(std::string_view name, std::function<bool()> fn);

using namespace flatearth;
using namespace flatearth::memory;
using namespace flatearth::containers;

#define ASSERT(expr) do { if (!(expr)) return false; } while(0)
#define PASS return true

static bool test_insert_and_retrieve() {
    MemoryManager mem;
    HashMap<uint32, int> map(mem);
    ASSERT(map.Insert(1u, 100).has_value());
    int* p = map.Retrieve(1u);
    ASSERT(p != nullptr && *p == 100);
    PASS;
}

static bool test_retrieve_missing_returns_null() {
    MemoryManager mem;
    HashMap<uint32, int> map(mem);
    ASSERT(map.Retrieve(42u) == nullptr);
    PASS;
}

static bool test_update_existing_key() {
    MemoryManager mem;
    HashMap<uint32, int> map(mem);
    map.Insert(1u, 10);
    map.Insert(1u, 20); // same key — update
    int* p = map.Retrieve(1u);
    ASSERT(p != nullptr && *p == 20);
    ASSERT(map.Size() == 1);
    PASS;
}

static bool test_erase_removes_entry() {
    MemoryManager mem;
    HashMap<uint32, int> map(mem);
    map.Insert(5u, 55);
    ASSERT(map.Erase(5u));
    ASSERT(map.Retrieve(5u) == nullptr);
    ASSERT(map.Size() == 0);
    PASS;
}

static bool test_erase_missing_returns_false() {
    MemoryManager mem;
    HashMap<uint32, int> map(mem);
    ASSERT(!map.Erase(99u));
    PASS;
}

static bool test_size_tracks_inserts_and_erases() {
    MemoryManager mem;
    HashMap<uint32, int> map(mem);
    map.Insert(1u, 1); map.Insert(2u, 2); map.Insert(3u, 3);
    ASSERT(map.Size() == 3);
    map.Erase(2u);
    ASSERT(map.Size() == 2);
    PASS;
}

static bool test_clear_empties_map() {
    MemoryManager mem;
    HashMap<uint32, int> map(mem);
    map.Insert(1u, 1); map.Insert(2u, 2);
    map.Clear();
    ASSERT(map.Size() == 0);
    ASSERT(map.Empty());
    ASSERT(map.Retrieve(1u) == nullptr);
    PASS;
}

static bool test_foreach_visits_all() {
    MemoryManager mem;
    HashMap<uint32, int> map(mem);
    map.Insert(1u, 10); map.Insert(2u, 20); map.Insert(3u, 30);
    int sum = 0;
    map.ForEach([&](uint32, const int& v) { sum += v; });
    ASSERT(sum == 60);
    PASS;
}

static bool test_move_value_insert() {
    using U32Array = DArray<uint32>;
    MemoryManager mem;
    HashMap<uint32, U32Array> hmap(mem);
    U32Array arr(mem);
    arr.Push(7u); arr.Push(8u);
    ASSERT(hmap.Insert(1u, std::move(arr)).has_value());
    U32Array* p = hmap.Retrieve(1u);
    ASSERT(p != nullptr && p->Length() == 2);
    ASSERT((*p)[0] == 7u && (*p)[1] == 8u);
    PASS;
}

namespace {
struct Reg {
    Reg() {
        RegisterTest("hashmap: insert and retrieve",         test_insert_and_retrieve);
        RegisterTest("hashmap: retrieve missing returns null", test_retrieve_missing_returns_null);
        RegisterTest("hashmap: update existing key",         test_update_existing_key);
        RegisterTest("hashmap: erase removes entry",         test_erase_removes_entry);
        RegisterTest("hashmap: erase missing returns false", test_erase_missing_returns_false);
        RegisterTest("hashmap: size tracks inserts/erases",  test_size_tracks_inserts_and_erases);
        RegisterTest("hashmap: clear empties map",           test_clear_empties_map);
        RegisterTest("hashmap: foreach visits all",          test_foreach_visits_all);
        RegisterTest("hashmap: move value insert",           test_move_value_insert);
    }
} gReg;
}
