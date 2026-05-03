#ifndef _FLATEARTH_ENGINE_PHYSICS_PHYSICS_HANDLE_HPP
#define _FLATEARTH_ENGINE_PHYSICS_PHYSICS_HANDLE_HPP

#include "Defines.hpp"

namespace flatearth::physics {

// Opaque handles matching box2d's b2WorldId / b2BodyId / b2ShapeId layout.
// Engine internals (PhysicsSystem.cc, FlatearthWorld.cc) cast via memcpy.
// Game code uses these directly — no box2d headers needed.

struct FeWorldHandle {
  uint16 index1{0};
  uint16 revision{0};
};

struct FeBodyHandle {
  int32 index1{0};
  uint16 world0{0};
  uint16 revision{0};
};

struct FeShapeHandle {
  int32 index1{0};
  uint16 world0{0};
  uint16 revision{0};
};

inline bool IsNull(FeWorldHandle h) {
  return h.index1 == 0;
}
inline bool IsNull(FeBodyHandle h) {
  return h.index1 == 0;
}
inline bool IsNull(FeShapeHandle h) {
  return h.index1 == 0;
}

} // namespace flatearth::physics

#endif // _FLATEARTH_ENGINE_PHYSICS_PHYSICS_HANDLE_HPP
