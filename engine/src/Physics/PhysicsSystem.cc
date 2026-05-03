#include "PhysicsSystem.hpp"

#include "Physics/BoxCollider.hpp"
#include "Physics/CircleCollider.hpp"
#include "Physics/FlatearthWorld.hpp"
#include "Physics/RigidBody.hpp"
#include "Scene/Components/Transform2D.hpp"
#include "box2d/box2d.h"

#include <cstring>

namespace flatearth::physics {

static b2WorldId ToB2World(FeWorldHandle h) {
  b2WorldId id;
  std::memcpy(&id, &h, sizeof(id));
  return id;
}

static b2BodyId ToB2Body(FeBodyHandle h) {
  b2BodyId id;
  std::memcpy(&id, &h, sizeof(id));
  return id;
}

static FeBodyHandle FromB2Body(b2BodyId id) {
  FeBodyHandle h;
  std::memcpy(&h, &id, sizeof(h));
  return h;
}

static b2ShapeId ToB2Shape(FeShapeHandle h) {
  b2ShapeId id;
  std::memcpy(&id, &h, sizeof(id));
  return id;
}

static FeShapeHandle FromB2Shape(b2ShapeId id) {
  FeShapeHandle h;
  std::memcpy(&h, &id, sizeof(h));
  return h;
}

PhysicsSystem::PhysicsSystem(FlatearthWorld &world) : _world(world) {
}

void PhysicsSystem::Update(ecs::Registry &registry, float32 deltaTime) {
  CreateBodies(registry);
  ProcessTeleports(registry);
  ApplyKinematicVelocities(registry);
  _world.Step(deltaTime);
  SyncTransforms(registry);
}

void PhysicsSystem::CreateBodies(ecs::Registry &registry) {
  auto view = registry.ViewOf<scene::Transform2D, RigidBody>();
  b2WorldId worldId = ToB2World(_world.WorldId());

  for (auto [entity, transform, body] : view) {
    if (!IsNull(body.bodyId)) {
      continue;
    }

    b2BodyDef def = b2DefaultBodyDef();
    def.position = {transform.worldX, transform.worldY};
    def.rotation = b2MakeRot(transform.worldRotation);
    def.type = body.type == BodyType::Static    ? b2_staticBody
               : body.type == BodyType::Dynamic ? b2_dynamicBody
                                                : b2_kinematicBody;
    def.linearDamping = body.linearDamping;
    def.angularDamping = body.angularDamping;
    def.fixedRotation = body.fixedRotation;

    b2BodyId b2Body = b2CreateBody(worldId, &def);
    body.bodyId = FromB2Body(b2Body);

    if (!body.velocity.Equals(math::Vec2D{0.0f, 0.0f})) {
      b2Body_SetLinearVelocity(b2Body, {body.velocity.x(), body.velocity.y()});
    }

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = body.density;
    shapeDef.friction = body.friction;
    shapeDef.restitution = body.restitution;

    if (registry.Has<BoxCollider>(entity)) {
      auto &collider = registry.Get<BoxCollider>(entity);
      shapeDef.isSensor = collider.isSensor;
      float32 angle = 0.0f;
      b2Polygon box = b2MakeOffsetBox(collider.halfWidth,
                                      collider.halfHeight,
                                      {collider.offset.x(), collider.offset.y()},
                                      angle);
      collider.shapeId = FromB2Shape(b2CreatePolygonShape(b2Body, &shapeDef, &box));
    }

    if (registry.Has<CircleCollider>(entity)) {
      auto &collider = registry.Get<CircleCollider>(entity);
      shapeDef.isSensor = collider.isSensor;
      b2Circle circle = {{collider.offset.x(), collider.offset.y()}, collider.radius};
      collider.shapeId = FromB2Shape(b2CreateCircleShape(b2Body, &shapeDef, &circle));
    }
  }
}

void PhysicsSystem::ProcessTeleports(ecs::Registry &registry) {
  auto view = registry.ViewOf<scene::Transform2D, RigidBody>();

  for (auto [entity, transform, body] : view) {
    if (!body.teleport || IsNull(body.bodyId)) {
      continue;
    }

    b2BodyId b2Body = ToB2Body(body.bodyId);
    b2Body_SetTransform(b2Body, {transform.x, transform.y}, b2MakeRot(transform.rotation));
    b2Body_SetLinearVelocity(b2Body, {body.velocity.x(), body.velocity.y()});
    b2Body_SetAngularVelocity(b2Body, 0.0f);
    body.teleport = FeFalse;
  }
}

void PhysicsSystem::ApplyKinematicVelocities(ecs::Registry &registry) {
  auto view = registry.ViewOf<scene::Transform2D, RigidBody>();

  for (auto [entity, transform, body] : view) {
    if (body.type != BodyType::Kinematic || IsNull(body.bodyId)) {
      continue;
    }
    b2Body_SetLinearVelocity(ToB2Body(body.bodyId), {body.velocity.x(), body.velocity.y()});
  }
}

void PhysicsSystem::SyncTransforms(ecs::Registry &registry) {
  auto view = registry.ViewOf<scene::Transform2D, RigidBody>();

  for (auto [entity, transform, body] : view) {
    if (IsNull(body.bodyId) || body.type == BodyType::Static) {
      continue;
    }

    b2BodyId b2Body = ToB2Body(body.bodyId);
    b2Vec2 pos = b2Body_GetPosition(b2Body);
    b2Rot rot = b2Body_GetRotation(b2Body);
    transform.x = pos.x;
    transform.y = pos.y;
    transform.rotation = b2Rot_GetAngle(rot);
    transform.dirty = FeTrue;
  }
}

} // namespace flatearth::physics
