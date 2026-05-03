#include "UIAnchor.hpp"

#include "ECS/Reflect/Reflect.hpp"

namespace flatearth::ui {

REFLECT_COMPONENT(UIAnchor)
FIELD(normalizedX, ecs::reflect::FieldType::Float32)
FIELD(normalizedY, ecs::reflect::FieldType::Float32)
FIELD(rotation, ecs::reflect::FieldType::Float32)
FIELD(scaleX, ecs::reflect::FieldType::Float32)
FIELD(scaleY, ecs::reflect::FieldType::Float32)
END_REFLECT

} // namespace flatearth::ui
