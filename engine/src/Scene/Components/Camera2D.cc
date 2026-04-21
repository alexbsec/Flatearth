#include "Camera2D.hpp"

#include "ECS/Reflect/FieldType.hpp"
#include "ECS/Reflect/Reflect.hpp"

namespace flatearth::scene {

REFLECT_COMPONENT(Camera2D)
FIELD(zoom, ecs::reflect::FieldType::Float32)
END_REFLECT

} // namespace flatearth::scene
