#include "ECS/Reflect/Serialize.hpp"

#include "Vendor/json.hpp"

#include <cstring>

namespace flatearth::ecs::reflect {

using json = nlohmann::json;

// ── field → json value ───────────────────────────────────────────────────────

static json FieldToJson(const void *ptr, FieldType type) {
  switch (type) {
    case FieldType::Null:
      return nullptr;
    case FieldType::Bool: {
      bool v;
      std::memcpy(&v, ptr, 1);
      return v;
    }
    case FieldType::Int8: {
      int8_t v;
      std::memcpy(&v, ptr, 1);
      return v;
    }
    case FieldType::Int16: {
      int16_t v;
      std::memcpy(&v, ptr, 2);
      return v;
    }
    case FieldType::Int32: {
      int32_t v;
      std::memcpy(&v, ptr, 4);
      return v;
    }
    case FieldType::Int64: {
      int64_t v;
      std::memcpy(&v, ptr, 8);
      return v;
    }
    case FieldType::Uint8: {
      uint8_t v;
      std::memcpy(&v, ptr, 1);
      return v;
    }
    case FieldType::Uint16: {
      uint16_t v;
      std::memcpy(&v, ptr, 2);
      return v;
    }
    case FieldType::Uint32: {
      uint32_t v;
      std::memcpy(&v, ptr, 4);
      return v;
    }
    case FieldType::Uint64: {
      uint64_t v;
      std::memcpy(&v, ptr, 8);
      return v;
    }
    case FieldType::Float32: {
      float v;
      std::memcpy(&v, ptr, 4);
      return v;
    }
    case FieldType::Float64: {
      double v;
      std::memcpy(&v, ptr, 8);
      return v;
    }
    case FieldType::Vec2D: {
      float x, y;
      std::memcpy(&x, ptr, 4);
      std::memcpy(&y, static_cast<const char *>(ptr) + 4, 4);
      return json{{"x", x}, {"y", y}};
    }
    case FieldType::String:
      return *static_cast<const string *>(ptr);
    case FieldType::CString:
      return static_cast<const char *>(ptr);
  }
  return nullptr;
}

// ── json value → field ───────────────────────────────────────────────────────

static bool FieldFromJson(void *ptr, FieldType type, const json &val, uint32 arraySize = 0) {
  try {
    switch (type) {
      case FieldType::Null:
        return false;
      case FieldType::Bool: {
        bool v = val.get<bool>();
        std::memcpy(ptr, &v, 1);
        return true;
      }
      case FieldType::Int8: {
        int8_t v = val.get<int8_t>();
        std::memcpy(ptr, &v, 1);
        return true;
      }
      case FieldType::Int16: {
        int16_t v = val.get<int16_t>();
        std::memcpy(ptr, &v, 2);
        return true;
      }
      case FieldType::Int32: {
        int32_t v = val.get<int32_t>();
        std::memcpy(ptr, &v, 4);
        return true;
      }
      case FieldType::Int64: {
        int64_t v = val.get<int64_t>();
        std::memcpy(ptr, &v, 8);
        return true;
      }
      case FieldType::Uint8: {
        uint8_t v = val.get<uint8_t>();
        std::memcpy(ptr, &v, 1);
        return true;
      }
      case FieldType::Uint16: {
        uint16_t v = val.get<uint16_t>();
        std::memcpy(ptr, &v, 2);
        return true;
      }
      case FieldType::Uint32: {
        uint32_t v = val.get<uint32_t>();
        std::memcpy(ptr, &v, 4);
        return true;
      }
      case FieldType::Uint64: {
        uint64_t v = val.get<uint64_t>();
        std::memcpy(ptr, &v, 8);
        return true;
      }
      case FieldType::Float32: {
        float v = val.get<float>();
        std::memcpy(ptr, &v, 4);
        return true;
      }
      case FieldType::Float64: {
        double v = val.get<double>();
        std::memcpy(ptr, &v, 8);
        return true;
      }
      case FieldType::Vec2D: {
        if (!val.is_object())
          return false;
        float x = val.at("x").get<float>();
        float y = val.at("y").get<float>();
        std::memcpy(ptr, &x, 4);
        std::memcpy(static_cast<char *>(ptr) + 4, &y, 4);
        return true;
      }
      case FieldType::String: {
        *static_cast<string *>(ptr) = val.get<string>();
        return true;
      }
      case FieldType::CString: {
        if (arraySize == 0) return false;
        string s = val.get<string>();
        std::strncpy(static_cast<char *>(ptr), s.c_str(), arraySize - 1);
        static_cast<char *>(ptr)[arraySize - 1] = '\0';
        return true;
      }
    }
  } catch (...) {
    return false;
  }
  return false;
}

// ── ToJson ───────────────────────────────────────────────────────────────────

string Serialize::ToJsonRaw(const void *component, const TypeDescriptor &desc) {
  json obj = json::object();
  for (const auto &f : desc.fields) {
    const void *ptr = static_cast<const char *>(component) + f.offset;
    obj[string(f.name)] = FieldToJson(ptr, f.type);
  }
  return obj.dump();
}

// ── FromJson ─────────────────────────────────────────────────────────────────

bool Serialize::FromJsonRaw(void *component, const TypeDescriptor &desc, stringv jsonStr) {
  json obj;
  try {
    obj = json::parse(jsonStr);
  } catch (...) {
    return false;
  }

  if (!obj.is_object())
    return false;

  for (const auto &f : desc.fields) {
    auto it = obj.find(string(f.name));
    if (it == obj.end())
      continue;
    void *ptr = static_cast<char *>(component) + f.offset;
    if (!FieldFromJson(ptr, f.type, *it, f.arraySize))
      return false;
  }
  return true;
}

// ── ToBinary ─────────────────────────────────────────────────────────────────
// Format: [typeId: u32][fieldCount: u32][field bytes in descriptor order...]
// Field order is position-dependent — descriptor order is the contract.
// TODO(Phase 8): delta compression, changed-field bitmask

std::vector<std::byte> Serialize::ToBinaryRaw(const void *component, const TypeDescriptor &desc) {
  std::vector<std::byte> buf;

  auto writeU32 = [&](uint32_t v) {
    const auto *p = reinterpret_cast<const std::byte *>(&v);
    buf.insert(buf.end(), p, p + 4);
  };

  writeU32(desc.typeId);
  writeU32(static_cast<uint32_t>(desc.fields.size()));

  for (const auto &f : desc.fields) {
    if (f.type == FieldType::String) {
      const string &s = *reinterpret_cast<const string *>(
          static_cast<const char *>(component) + f.offset);
      writeU32(static_cast<uint32_t>(s.size()));
      const auto *src = reinterpret_cast<const std::byte *>(s.data());
      buf.insert(buf.end(), src, src + s.size());
    } else if (f.type == FieldType::CString) {
      const char *cs = static_cast<const char *>(component) + f.offset;
      uint32_t len = static_cast<uint32_t>(std::strlen(cs));
      writeU32(len);
      const auto *src = reinterpret_cast<const std::byte *>(cs);
      buf.insert(buf.end(), src, src + len);
    } else {
      uint8_t size = FieldSize(f.type);
      const auto *src =
          reinterpret_cast<const std::byte *>(static_cast<const char *>(component) + f.offset);
      buf.insert(buf.end(), src, src + size);
    }
  }

  return buf;
}

// ── FromBinary ───────────────────────────────────────────────────────────────

bool Serialize::FromBinaryRaw(void *component,
                              const TypeDescriptor &desc,
                              const std::vector<std::byte> &data) {
  size_t cursor = 0;

  auto readU32 = [&](uint32_t &out) -> bool {
    if (cursor + 4 > data.size())
      return false;
    std::memcpy(&out, data.data() + cursor, 4);
    cursor += 4;
    return true;
  };

  uint32_t typeId, fieldCount;
  if (!readU32(typeId) || !readU32(fieldCount))
    return false;
  if (typeId != desc.typeId)
    return false;
  if (fieldCount != static_cast<uint32_t>(desc.fields.size()))
    return false;

  for (const auto &f : desc.fields) {
    if (f.type == FieldType::String) {
      uint32_t len;
      if (!readU32(len)) return false;
      if (cursor + len > data.size()) return false;
      string *s = reinterpret_cast<string *>(static_cast<char *>(component) + f.offset);
      *s = string(reinterpret_cast<const char *>(data.data() + cursor), len);
      cursor += len;
    } else if (f.type == FieldType::CString) {
      uint32_t len;
      if (!readU32(len)) return false;
      if (cursor + len > data.size()) return false;
      char *dst = static_cast<char *>(component) + f.offset;
      uint32_t cap = f.arraySize > 0 ? f.arraySize : len + 1;
      uint32_t copy = len < cap ? len : cap - 1;
      std::memcpy(dst, data.data() + cursor, copy);
      dst[copy] = '\0';
      cursor += len;
    } else {
      uint8_t size = FieldSize(f.type);
      if (cursor + size > data.size())
        return false;
      void *dst = static_cast<char *>(component) + f.offset;
      std::memcpy(dst, data.data() + cursor, size);
      cursor += size;
    }
  }

  return true;
}

} // namespace flatearth::ecs::reflect
