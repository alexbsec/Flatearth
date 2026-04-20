#include "ECS/Reflect/Authority.hpp"
#include "ECS/Reflect/Reflect.hpp"

#include <string_view>
#include <functional>

void RegisterTest(std::string_view name, std::function<bool()> fn);

using namespace flatearth;
using namespace flatearth::ecs::reflect;

#define ASSERT(expr) do { if (!(expr)) return false; } while(0)
#define PASS return true

// ── test components ──────────────────────────────────────────────────────────

struct AuthLocalComp  { float x; };
struct AuthServerComp { float x; float y; };
struct AuthClientComp { int32_t score; };
struct AuthSharedComp { bool flag; };

REFLECT_COMPONENT(AuthLocalComp)
    FIELD(x, FieldType::Float32)
END_REFLECT

REFLECT_COMPONENT(AuthServerComp, Authority::Server)
    FIELD(x, FieldType::Float32)
    FIELD(y, FieldType::Float32)
END_REFLECT

REFLECT_COMPONENT(AuthClientComp, Authority::Client)
    FIELD(score, FieldType::Int32)
END_REFLECT

REFLECT_COMPONENT(AuthSharedComp, Authority::Shared)
    FIELD(flag, FieldType::Bool)
END_REFLECT

// ── tests ────────────────────────────────────────────────────────────────────

static bool test_default_authority_is_local() {
    const TypeDescriptor *desc = TypeRegistry::Get<AuthLocalComp>();
    ASSERT(desc != nullptr);
    ASSERT(desc->authority == Authority::Local);
    PASS;
}

static bool test_server_authority_stored() {
    const TypeDescriptor *desc = TypeRegistry::Get<AuthServerComp>();
    ASSERT(desc != nullptr);
    ASSERT(desc->authority == Authority::Server);
    PASS;
}

static bool test_client_authority_stored() {
    const TypeDescriptor *desc = TypeRegistry::Get<AuthClientComp>();
    ASSERT(desc != nullptr);
    ASSERT(desc->authority == Authority::Client);
    PASS;
}

static bool test_shared_authority_stored() {
    const TypeDescriptor *desc = TypeRegistry::Get<AuthSharedComp>();
    ASSERT(desc != nullptr);
    ASSERT(desc->authority == Authority::Shared);
    PASS;
}

static bool test_authority_name_strings() {
    ASSERT(std::string_view(AuthorityName(Authority::Local))  == "Local");
    ASSERT(std::string_view(AuthorityName(Authority::Server)) == "Server");
    ASSERT(std::string_view(AuthorityName(Authority::Client)) == "Client");
    ASSERT(std::string_view(AuthorityName(Authority::Shared)) == "Shared");
    PASS;
}

static bool test_authority_does_not_affect_fields() {
    const TypeDescriptor *desc = TypeRegistry::Get<AuthServerComp>();
    ASSERT(desc != nullptr);
    ASSERT(desc->fields.size() == 2);
    ASSERT(desc->fields[0].name == "x");
    ASSERT(desc->fields[1].name == "y");
    PASS;
}

// ── registration ─────────────────────────────────────────────────────────────

namespace {
struct Reg {
    Reg() {
        RegisterTest("authority: default is Local",          test_default_authority_is_local);
        RegisterTest("authority: Server stored correctly",   test_server_authority_stored);
        RegisterTest("authority: Client stored correctly",   test_client_authority_stored);
        RegisterTest("authority: Shared stored correctly",   test_shared_authority_stored);
        RegisterTest("authority: AuthorityName strings",     test_authority_name_strings);
        RegisterTest("authority: fields unaffected by auth", test_authority_does_not_affect_fields);
    }
} gReg;
}
