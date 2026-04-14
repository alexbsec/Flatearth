#include "MeshSystem.hpp"

#include "Core/Logger.hpp"
#include "Math/MathTypes.hpp"

#include <cmath>
#include <vector>

namespace flatearth::resources {

static constexpr float32 scPi = 3.14159265358979f;

static const char *ShapeToString(MeshShape shape) {
  switch (shape) {
    case MeshShape::Quad:
      return "quad";
    case MeshShape::Circle:
      return "circle";
    case MeshShape::Capsule2D:
      return "capsule2d";
  }
  return "unknown";
}

static void GenerateQuad(std::vector<math::Vertex3D> &verts, std::vector<uint32> &indices) {
  verts = {
      {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}},
  };
  indices = {0, 2, 1, 0, 3, 2};
}

static void GenerateCircle(std::vector<math::Vertex3D> &verts, std::vector<uint32> &indices) {
  constexpr uint32 n = 32;
  constexpr float32 r = 0.5f;

  // center
  verts.push_back({{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});

  // edge vertices
  for (uint32 i = 0; i < n; i++) {
    float32 a = 2.0f * scPi * static_cast<float32>(i) / n;
    float32 x = r * std::cos(a);
    float32 y = r * std::sin(a);
    verts.push_back({{x, y, 0.0f}, {0.5f + x, 0.5f - y}});
  }

  // fan triangles from center
  for (uint32 i = 0; i < n; i++) {
    indices.push_back(0);
    indices.push_back(1 + i);
    indices.push_back(1 + (i + 1) % n);
  }
}

static void GenerateCapsule2D(std::vector<math::Vertex3D> &verts, std::vector<uint32> &indices) {
  constexpr uint32 n = 8; // segments per semicircle
  constexpr float32 r = 0.25f;
  constexpr float32 h = 0.25f; // half body height
  constexpr float32 totalH = h + r;

  // center
  verts.push_back({{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});

  // top semicircle: angles 0..π, center at (0, +h)
  for (uint32 i = 0; i <= n; i++) {
    float32 a = scPi * static_cast<float32>(i) / n;
    float32 x = r * std::cos(a);
    float32 y = h + r * std::sin(a);
    verts.push_back({{x, y, 0.0f}, {(x + r) / (2.0f * r), 1.0f - (y + totalH) / (2.0f * totalH)}});
  }

  // bottom semicircle: angles π..2π, center at (0, -h)
  for (uint32 i = 0; i <= n; i++) {
    float32 a = scPi + scPi * static_cast<float32>(i) / n;
    float32 x = r * std::cos(a);
    float32 y = -h + r * std::sin(a);
    verts.push_back({{x, y, 0.0f}, {(x + r) / (2.0f * r), 1.0f - (y + totalH) / (2.0f * totalH)}});
  }

  // fan triangles from center — covers caps and body side segments
  uint32 outlineCount = 2 * (n + 1);
  for (uint32 i = 0; i < outlineCount; i++) {
    indices.push_back(0);
    indices.push_back(1 + i);
    indices.push_back(1 + (i + 1) % outlineCount);
  }
}

// ----

MeshSystem::MeshSystem(memory::MemoryManager &memManager, renderer::FrontendRenderer &renderer)
    : ResourceSystem(memManager), _renderer(renderer) {
}

FeExpect<MeshHandle, Error> MeshSystem::AcquireMesh(MeshShape shape) {
  _pendingShape = shape;
  return this->ProtectedAcquire(ShapeToString(shape));
}

FeExpect<void, Error> MeshSystem::Create(Mesh *pMesh, uint32 handle, const string &name) {
  std::vector<math::Vertex3D> verts;
  std::vector<uint32> indices;

  switch (_pendingShape) {
    case MeshShape::Quad:
      GenerateQuad(verts, indices);
      break;
    case MeshShape::Circle:
      GenerateCircle(verts, indices);
      break;
    case MeshShape::Capsule2D:
      GenerateCapsule2D(verts, indices);
      break;
  }

  pMesh->id = handle;
  pMesh->name = name;
  pMesh->vertexCount = static_cast<uint32>(verts.size());
  pMesh->indexCount = static_cast<uint32>(indices.size());

  return _renderer.CreateGeometry(
      handle, pMesh->vertexCount, verts.data(), pMesh->indexCount, indices.data());
}

FeExpect<void, Error> MeshSystem::Destroy(Mesh *pMesh) {
  return _renderer.DestroyGeometry(pMesh->id);
}

} // namespace flatearth::resources
