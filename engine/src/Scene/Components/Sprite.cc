#include "Sprite.hpp"

#include "ECS/Reflect/FieldType.hpp"
#include "ECS/Reflect/Reflect.hpp"

namespace flatearth::scene {

REFLECT_COMPONENT(Sprite)
FIELD(layer, ecs::reflect::FieldType::Uint32)
FIELD(meshShape, ecs::reflect::FieldType::Uint16)
FIELD(uvOffset, ecs::reflect::FieldType::Vec2D)
FIELD(uvScale, ecs::reflect::FieldType::Vec2D)
FIELD(flipX, ecs::reflect::FieldType::Bool)
FIELD(flipY, ecs::reflect::FieldType::Bool)
END_REFLECT

} // namespace flatearth::scene
