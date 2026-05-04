#include "UIButton.hpp"

#include "ECS/Reflect/FieldType.hpp"
#include "ECS/Reflect/Reflect.hpp"

namespace flatearth::ui {

REFLECT_COMPONENT(UIButton)
FIELD(normalTint, ecs::reflect::FieldType::Vec3D)
FIELD(hoverTint, ecs::reflect::FieldType::Vec3D)
FIELD(pressedTint, ecs::reflect::FieldType::Vec3D)
FIELD(disabledTint, ecs::reflect::FieldType::Vec3D)
END_REFLECT

} // namespace flatearth::ui
