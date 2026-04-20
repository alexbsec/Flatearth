#include "Transform2D.hpp"

#include "ECS/Reflect/Reflect.hpp"

namespace flatearth::scene {

REFLECT_COMPONENT(Transform2D)
FIELD(x, ecs::reflect::FieldType::Float32)
FIELD(y, ecs::reflect::FieldType::Float32)
FIELD(rotation, ecs::reflect::FieldType::Float32)
FIELD(scaleX, ecs::reflect::FieldType::Float32)
FIELD(scaleY, ecs::reflect::FieldType::Float32)
END_REFLECT

} // namespace flatearth::scene
