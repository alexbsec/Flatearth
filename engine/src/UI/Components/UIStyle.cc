#include "UIStyle.hpp"

#include "ECS/Reflect/Reflect.hpp"

namespace flatearth::ui {

REFLECT_COMPONENT(UIStyle)
FIELD(tint, ecs::reflect::FieldType::Vec3D)
FIELD(uvOffset, ecs::reflect::FieldType::Vec2D)
FIELD(uvScale, ecs::reflect::FieldType::Vec2D)
FIELD(useTexture, ecs::reflect::FieldType::Float32)
END_REFLECT

} // namespace flatearth::ui
