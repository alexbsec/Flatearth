#include "Children.hpp"
#include "ECS/ECSTypes.hpp"

namespace flatearth::scene {

bool Children::Add(ecs::EntityId id) {
  if (this->count >= this->scMaxChildren || Has(id)) {
    return FeFalse;
  }
  this->ids[this->count++] = id;
  return FeTrue;
}

bool Children::Remove(ecs::EntityId id) {
  for (uint8 i = 0; i < this->count; i++) {
    if (this->ids[i] != id) {
      continue;
    }

    this->ids[i] = ids[--this->count];
    return FeTrue;
  }

  return FeFalse;
}

bool Children::Has(ecs::EntityId id) const {
  for (uint8 i = 0; i < this->count; i++) {
    if (this->ids[i] == id) {
      return FeTrue;
    }
  }

  return FeFalse;
}


}
