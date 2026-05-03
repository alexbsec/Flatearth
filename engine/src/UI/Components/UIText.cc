#include "UIText.hpp"

#include "ECS/Reflect/Reflect.hpp"

namespace flatearth::ui {

REFLECT_COMPONENT(UIText)
FIELD_CSTR(text, cMaxTextLength)
FIELD(color, ecs::reflect::FieldType::Vec3D)
END_REFLECT

} // namespace flatearth::ui
