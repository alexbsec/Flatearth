#ifndef _FLATEARTH_ENGINE_ECS_REFLECT_AUTHORITY_HPP
#define _FLATEARTH_ENGINE_ECS_REFLECT_AUTHORITY_HPP

#include "Defines.hpp"

namespace flatearth::ecs::reflect {

// Ownership model for networked components.
//
// Local  — never replicated; exists only on the machine that owns it.
// Server — server is authoritative; clients receive updates, never write.
// Client — owning client is authoritative; server relays, others receive.
// Shared — any peer may write; last-write-wins (use sparingly).
//
// Conflict resolution (Phase 8): Server > Client > Shared > Local.
// Rollback interaction: only Server and Client components are included in
// ECS snapshots used by the rollback system; Local components are skipped.
enum class Authority : uint8 {
  Local = 0,
  Server = 1,
  Client = 2,
  Shared = 3,
};

constexpr const char *AuthorityName(Authority a) {
  switch (a) {
    case Authority::Local:
      return "Local";
    case Authority::Client:
      return "Client";
    case Authority::Server:
      return "Server";
    case Authority::Shared:
      return "Shared";
  }

  return "Unknown";
}

} // namespace flatearth::ecs::reflect

#endif // _FLATEARTH_ENGINE_ECS_REFLECT_AUTHORITY_HPP
