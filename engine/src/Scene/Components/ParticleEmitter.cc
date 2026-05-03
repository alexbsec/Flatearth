#include "ParticleEmitter.hpp"

#include "ECS/Reflect/FieldType.hpp"
#include "ECS/Reflect/Reflect.hpp"

namespace flatearth::scene {

REFLECT_COMPONENT(ParticleEmitter)
FIELD(velocityMin, ecs::reflect::FieldType::Vec2D)
FIELD(velocityMax, ecs::reflect::FieldType::Vec2D)
FIELD(lifetimeMin, ecs::reflect::FieldType::Float32)
FIELD(lifetimeMax, ecs::reflect::FieldType::Float32)
FIELD(sizeStart, ecs::reflect::FieldType::Float32)
FIELD(sizeEnd, ecs::reflect::FieldType::Float32)
FIELD(spawnRate, ecs::reflect::FieldType::Float32)
FIELD(layer, ecs::reflect::FieldType::Uint32)
END_REFLECT

} // namespace flatearth::scene
