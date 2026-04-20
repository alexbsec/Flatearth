#include "ECS/Reflect/Reflect.hpp"
#include "ECS/Reflect/Serialize.hpp"

#include <cstring>
#include <string_view>
#include <functional>

void RegisterTest(std::string_view name, std::function<bool()> fn);

using namespace flatearth;
using namespace flatearth::ecs::reflect;

#define ASSERT(expr) do { if (!(expr)) return false; } while(0)
#define PASS return true

// ── test components ──────────────────────────────────────────────────────────

struct Vec2Component { float x, y; };
struct StatsComponent { int32_t hp; uint32_t level; bool alive; };
struct MixedComponent { float f32; double f64; int8_t i8; uint64_t u64; };

REFLECT_COMPONENT(Vec2Component)
    FIELD(x, FieldType::Float32)
    FIELD(y, FieldType::Float32)
END_REFLECT

REFLECT_COMPONENT(StatsComponent)
    FIELD(hp,    FieldType::Int32)
    FIELD(level, FieldType::Uint32)
    FIELD(alive, FieldType::Bool)
END_REFLECT

REFLECT_COMPONENT(MixedComponent)
    FIELD(f32, FieldType::Float32)
    FIELD(f64, FieldType::Float64)
    FIELD(i8,  FieldType::Int8)
    FIELD(u64, FieldType::Uint64)
END_REFLECT

// ── TypeRegistry tests ───────────────────────────────────────────────────────

static bool test_registry_get_by_type() {
    const TypeDescriptor* desc = TypeRegistry::Get<Vec2Component>();
    ASSERT(desc != nullptr);
    ASSERT(desc->name == "Vec2Component");
    ASSERT(desc->fields.size() == 2);
    PASS;
}

static bool test_registry_get_by_name() {
    const TypeDescriptor* desc = TypeRegistry::Get<StatsComponent>("StatsComponent");
    ASSERT(desc != nullptr);
    ASSERT(desc->name == "StatsComponent");
    PASS;
}

static bool test_registry_unknown_name_returns_null() {
    ASSERT(TypeRegistry::Get<Vec2Component>("DoesNotExist") == nullptr);
    PASS;
}

static bool test_registry_field_names_and_offsets() {
    const TypeDescriptor* desc = TypeRegistry::Get<Vec2Component>();
    ASSERT(desc != nullptr);
    ASSERT(desc->fields[0].name == "x");
    ASSERT(desc->fields[0].offset == offsetof(Vec2Component, x));
    ASSERT(desc->fields[0].type == FieldType::Float32);
    ASSERT(desc->fields[1].name == "y");
    ASSERT(desc->fields[1].offset == offsetof(Vec2Component, y));
    PASS;
}

static bool test_registry_type_id_stable() {
    const TypeDescriptor* a = TypeRegistry::Get<StatsComponent>();
    const TypeDescriptor* b = TypeRegistry::Get<StatsComponent>();
    ASSERT(a != nullptr && b != nullptr);
    ASSERT(a->typeId == b->typeId);
    PASS;
}

static bool test_registry_different_types_different_ids() {
    const TypeDescriptor* a = TypeRegistry::Get<Vec2Component>();
    const TypeDescriptor* b = TypeRegistry::Get<StatsComponent>();
    ASSERT(a != nullptr && b != nullptr);
    ASSERT(a->typeId != b->typeId);
    PASS;
}

// ── JSON round-trip tests ────────────────────────────────────────────────────

static bool test_json_vec2_roundtrip() {
    Vec2Component src{3.14f, -2.71f};
    string json = Serialize::ToJson(src);

    Vec2Component dst{};
    ASSERT(Serialize::FromJson(dst, json));
    ASSERT(dst.x == src.x);
    ASSERT(dst.y == src.y);
    PASS;
}

static bool test_json_stats_roundtrip() {
    StatsComponent src{100, 5u, true};
    string json = Serialize::ToJson(src);

    StatsComponent dst{};
    ASSERT(Serialize::FromJson(dst, json));
    ASSERT(dst.hp    == src.hp);
    ASSERT(dst.level == src.level);
    ASSERT(dst.alive == src.alive);
    PASS;
}

static bool test_json_mixed_roundtrip() {
    MixedComponent src{1.5f, 3.14159265358979, -42, 99999999999ull};
    string json = Serialize::ToJson(src);

    MixedComponent dst{};
    ASSERT(Serialize::FromJson(dst, json));
    ASSERT(dst.f32 == src.f32);
    ASSERT(dst.f64 == src.f64);
    ASSERT(dst.i8  == src.i8);
    ASSERT(dst.u64 == src.u64);
    PASS;
}

static bool test_json_unregistered_type_returns_empty() {
    struct Ghost { float x; }; // not reflected
    Ghost g{1.f};
    ASSERT(Serialize::ToJson(g) == "{}");
    PASS;
}

static bool test_json_malformed_returns_false() {
    Vec2Component dst{};
    ASSERT(!Serialize::FromJson(dst, "not json at all {{"));
    PASS;
}

static bool test_json_partial_fields_leaves_rest_default() {
    Vec2Component dst{10.f, 20.f};
    // JSON only has x — y should stay as-is
    ASSERT(Serialize::FromJson(dst, R"({"x":5.0})"));
    ASSERT(dst.x == 5.f);
    ASSERT(dst.y == 20.f);
    PASS;
}

// ── Binary round-trip tests ──────────────────────────────────────────────────

static bool test_binary_vec2_roundtrip() {
    Vec2Component src{1.f, 2.f};
    auto buf = Serialize::ToBinary(src);
    ASSERT(!buf.empty());

    Vec2Component dst{};
    ASSERT(Serialize::FromBinary(dst, buf));
    ASSERT(dst.x == src.x);
    ASSERT(dst.y == src.y);
    PASS;
}

static bool test_binary_stats_roundtrip() {
    StatsComponent src{-50, 99u, false};
    auto buf = Serialize::ToBinary(src);

    StatsComponent dst{};
    ASSERT(Serialize::FromBinary(dst, buf));
    ASSERT(dst.hp    == src.hp);
    ASSERT(dst.level == src.level);
    ASSERT(dst.alive == src.alive);
    PASS;
}

static bool test_binary_header_has_type_id() {
    Vec2Component src{};
    auto buf = Serialize::ToBinary(src);
    ASSERT(buf.size() >= 8); // typeId(4) + fieldCount(4)

    uint32_t typeId;
    std::memcpy(&typeId, buf.data(), 4);
    ASSERT(typeId == TypeRegistry::Get<Vec2Component>()->typeId);
    PASS;
}

static bool test_binary_wrong_type_id_returns_false() {
    Vec2Component src{1.f, 2.f};
    auto buf = Serialize::ToBinary(src);

    // corrupt typeId
    uint32_t bad = 0xdeadbeef;
    std::memcpy(buf.data(), &bad, 4);

    Vec2Component dst{};
    ASSERT(!Serialize::FromBinary(dst, buf));
    PASS;
}

static bool test_binary_truncated_returns_false() {
    Vec2Component src{1.f, 2.f};
    auto buf = Serialize::ToBinary(src);
    buf.resize(buf.size() / 2); // chop it

    Vec2Component dst{};
    ASSERT(!Serialize::FromBinary(dst, buf));
    PASS;
}

static bool test_binary_unregistered_type_returns_empty() {
    struct Ghost { float x; };
    Ghost g{1.f};
    ASSERT(Serialize::ToBinary(g).empty());
    PASS;
}

// ── registration ─────────────────────────────────────────────────────────────

namespace {
struct Reg {
    Reg() {
        RegisterTest("reflect: registry get by type",                test_registry_get_by_type);
        RegisterTest("reflect: registry get by name",                test_registry_get_by_name);
        RegisterTest("reflect: registry unknown name returns null",  test_registry_unknown_name_returns_null);
        RegisterTest("reflect: registry field names and offsets",    test_registry_field_names_and_offsets);
        RegisterTest("reflect: registry type id stable",             test_registry_type_id_stable);
        RegisterTest("reflect: registry different types differ ids", test_registry_different_types_different_ids);
        RegisterTest("reflect: json vec2 roundtrip",                 test_json_vec2_roundtrip);
        RegisterTest("reflect: json stats roundtrip",                test_json_stats_roundtrip);
        RegisterTest("reflect: json mixed roundtrip",                test_json_mixed_roundtrip);
        RegisterTest("reflect: json unregistered type empty",        test_json_unregistered_type_returns_empty);
        RegisterTest("reflect: json malformed returns false",        test_json_malformed_returns_false);
        RegisterTest("reflect: json partial fields leaves rest",     test_json_partial_fields_leaves_rest_default);
        RegisterTest("reflect: binary vec2 roundtrip",               test_binary_vec2_roundtrip);
        RegisterTest("reflect: binary stats roundtrip",              test_binary_stats_roundtrip);
        RegisterTest("reflect: binary header has type id",           test_binary_header_has_type_id);
        RegisterTest("reflect: binary wrong type id returns false",  test_binary_wrong_type_id_returns_false);
        RegisterTest("reflect: binary truncated returns false",      test_binary_truncated_returns_false);
        RegisterTest("reflect: binary unregistered type empty",      test_binary_unregistered_type_returns_empty);
    }
} gReg;
}
