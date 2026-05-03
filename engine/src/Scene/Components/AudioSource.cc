#include "AudioSource.hpp"

#include "ECS/Reflect/Reflect.hpp"

namespace flatearth::scene {

REFLECT_COMPONENT(AudioSource)
FIELD_CSTR(path, cAudioPathMax)
FIELD(loop, ecs::reflect::FieldType::Bool)
FIELD(playing, ecs::reflect::FieldType::Bool)
FIELD(volume, ecs::reflect::FieldType::Float32)
END_REFLECT

} // namespace flatearth::scene
