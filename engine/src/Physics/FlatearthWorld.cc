#include "FlatearthWorld.hpp"
#include "box2d/box2d.h"

#include <cstring>

namespace flatearth::physics {

static b2WorldId ToB2(FeWorldHandle h) {
  b2WorldId id;
  std::memcpy(&id, &h, sizeof(id));
  return id;
}

static FeWorldHandle FromB2(b2WorldId id) {
  FeWorldHandle h;
  std::memcpy(&h, &id, sizeof(h));
  return h;
}

FlatearthWorld::FlatearthWorld(memory::MemoryManager &memManager) {}

FeWorldHandle FlatearthWorld::WorldId() const { return _worldHandle; }

void FlatearthWorld::Initialize(float32 gravityX, float32 gravityY) {
  if (_initialized) {
    return;
  }

  b2WorldDef def = b2DefaultWorldDef();
  def.gravity = {gravityX, gravityY};
  _worldHandle = FromB2(b2CreateWorld(&def));
  _initialized = FeTrue;
}

void FlatearthWorld::Step(float32 deltaTime, int32 subSteps) {
  b2World_Step(ToB2(_worldHandle), deltaTime, subSteps);
}

void FlatearthWorld::Shutdown() {
  b2WorldId id = ToB2(_worldHandle);
  if (B2_IS_NON_NULL(id)) {
    b2DestroyWorld(id);
    _worldHandle = {};
    _initialized = FeFalse;
  }
}

} // namespace flatearth::physics
