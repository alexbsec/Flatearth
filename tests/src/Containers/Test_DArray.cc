#include "Containers/DArray.hpp"
#include "Core/FeMemory.hpp"

#include <string_view>
#include <functional>

void RegisterTest(std::string_view name, std::function<bool()> fn);

using namespace flatearth;
using namespace flatearth::memory;
using namespace flatearth::containers;

#define ASSERT(expr) do { if (!(expr)) return false; } while(0)
#define PASS return true

static bool test_push_and_length() {
    MemoryManager mem;
    DArray<int> arr(mem);
    ASSERT(arr.Length() == 0);
    arr.Push(1); arr.Push(2); arr.Push(3);
    ASSERT(arr.Length() == 3);
    ASSERT(arr[0] == 1 && arr[1] == 2 && arr[2] == 3);
    PASS;
}

static bool test_pop_decrements_length() {
    MemoryManager mem;
    DArray<int> arr(mem);
    arr.Push(10); arr.Push(20);
    arr.Pop();
    ASSERT(arr.Length() == 1);
    ASSERT(arr[0] == 10);
    PASS;
}

static bool test_clear_resets_length() {
    MemoryManager mem;
    DArray<int> arr(mem);
    arr.Push(1); arr.Push(2);
    arr.Clear();
    ASSERT(arr.Length() == 0);
    ASSERT(arr.Empty());
    PASS;
}

static bool test_reserve_grows_capacity() {
    MemoryManager mem;
    DArray<int> arr(mem);
    arr.Reserve(128);
    ASSERT(arr.Capacity() >= 128);
    ASSERT(arr.Length() == 0);
    PASS;
}

static bool test_resize_sets_length() {
    MemoryManager mem;
    DArray<int> arr(mem);
    arr.Resize(8);
    ASSERT(arr.Length() == 8);
    ASSERT(arr.Capacity() >= 8);
    PASS;
}

static bool test_auto_grows_on_push() {
    MemoryManager mem;
    DArray<int> arr(mem);
    arr.Reserve(1); // shrink headroom so resize happens multiple times
    for (int i = 0; i < 16; ++i)
        arr.Push(i);
    ASSERT(arr.Length() == 16);
    for (int i = 0; i < 16; ++i)
        ASSERT(arr[i] == i);
    PASS;
}

static bool test_insert_at() {
    MemoryManager mem;
    DArray<int> arr(mem);
    arr.Push(1); arr.Push(3);
    arr.InsertAt(2, 1); // insert 2 at index 1
    ASSERT(arr.Length() == 3);
    ASSERT(arr[0] == 1 && arr[1] == 2 && arr[2] == 3);
    PASS;
}

static bool test_pop_at() {
    MemoryManager mem;
    DArray<int> arr(mem);
    arr.Push(10); arr.Push(20); arr.Push(30);
    arr.PopAt(1);
    ASSERT(arr.Length() == 2);
    ASSERT(arr[0] == 10 && arr[1] == 30);
    PASS;
}

static bool test_move_constructor() {
    MemoryManager mem;
    DArray<int> a(mem);
    a.Push(7); a.Push(8); a.Push(9);
    DArray<int> b(std::move(a));
    ASSERT(b.Length() == 3);
    ASSERT(b[0] == 7 && b[1] == 8 && b[2] == 9);
    ASSERT(a.Length() == 0);
    PASS;
}

static bool test_move_assignment() {
    MemoryManager mem;
    DArray<int> a(mem);
    a.Push(1); a.Push(2);
    DArray<int> b(mem);
    b.Push(99);
    b = std::move(a);
    ASSERT(b.Length() == 2);
    ASSERT(b[0] == 1 && b[1] == 2);
    PASS;
}

namespace {
struct Reg {
    Reg() {
        RegisterTest("darray: push and length",       test_push_and_length);
        RegisterTest("darray: pop decrements length", test_pop_decrements_length);
        RegisterTest("darray: clear resets length",   test_clear_resets_length);
        RegisterTest("darray: reserve grows capacity",test_reserve_grows_capacity);
        RegisterTest("darray: resize sets length",    test_resize_sets_length);
        RegisterTest("darray: auto grows on push",    test_auto_grows_on_push);
        RegisterTest("darray: insert at",             test_insert_at);
        RegisterTest("darray: pop at",                test_pop_at);
        RegisterTest("darray: move constructor",      test_move_constructor);
        RegisterTest("darray: move assignment",       test_move_assignment);
    }
} gReg;
}
