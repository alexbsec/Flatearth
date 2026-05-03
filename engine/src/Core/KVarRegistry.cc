#include "KVarRegistry.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"

#include <variant>

namespace flatearth {

KVarRegistry::KVarRegistry(memory::MemoryManager &memManager) : _localMap(memManager) {
}

FeExpect<void, Error> KVarRegistry::Register(stringv name, stringv desc, KVarValue defaultVal) {
  const string key{name};
  const string description{desc};

  if (_localMap.Has(key)) {
    const string errMsg = std::format("KVar already registered: {}", key);
    return FeErr{Error(errMsg, ErrorType::SystemError)};
  }

  KVar kvar{};
  kvar.name = key;
  kvar.description = description;
  kvar.value = defaultVal;
  kvar.defaultValue = defaultVal;

  auto insertRes = _localMap.Insert(key, kvar);
  if (insertRes.errored()) {
    FLOG_ERROR("inserting new kvar failed");
    return FeErr{insertRes.error()};
  }

  return {};
}

FeExpect<bool, Error> KVarRegistry::ParseAndSet(stringv name, stringv strValue) {
  const string key{name};

  if (!_localMap.Has(key)) {
    const string errMsg = std::format("KVar not found for key {}", key);
    return FeErr{Error(errMsg, ErrorType::SystemError)};
  }

  KVar *pKvar = _localMap.Retrieve(key);
  if (pKvar == nullptr) {
    return _cRetrieveErr;
  }

  const KVarValue oldValue = pKvar->value;

  auto parseRes = ParseKVarValue(strValue, *pKvar);
  if (parseRes.errored()) {
    return FeErr{parseRes.error()};
  }

  const bool changed = (pKvar->value != oldValue);
  if (changed && pKvar->onChange != nullptr) {
    pKvar->onChange(pKvar->value);
  }

  return changed;
}

string KVarRegistry::GetAsString(stringv name) const {
  const string key{name};
  const KVar *pKvar = _localMap.Retrieve(key);
  if (pKvar == nullptr)
    return "<not found>";

  return std::visit(
      [](const auto &v) -> string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>)
          return v ? "true" : "false";
        if constexpr (std::is_same_v<T, int32>)
          return std::to_string(v);
        if constexpr (std::is_same_v<T, float32>)
          return std::to_string(v);
        if constexpr (std::is_same_v<T, string>)
          return v;
        return "<unknown>";
      },
      pKvar->value);
}

FeExpect<void, Error> KVarRegistry::ParseKVarValue(stringv strValue, KVar &outVar) {
  if (std::holds_alternative<bool>(outVar.value)) {
    bool parsedValue{};
    const string raw{strValue};

    if (raw == "true" || raw == "1") {
      parsedValue = true;
    } else if (raw == "false" || raw == "0") {
      parsedValue = false;
    } else {
      return FeErr{Error("Invalid bool value for KVar: " + raw, ErrorType::SystemError)};
    }

    outVar.value = parsedValue;
  } else if (std::holds_alternative<int32>(outVar.value)) {
    int32 parsedValue{};
    const char *begin = strValue.data();
    const char *end = strValue.data() + strValue.size();

    auto [ptr, ec] = std::from_chars(begin, end, parsedValue);
    if (ec != std::errc{} || ptr != end) {
      return FeErr{
          Error("Invalid int32 value for KVar: " + string{strValue}, ErrorType::SystemError)};
    }

    outVar.value = parsedValue;
  } else if (std::holds_alternative<float32>(outVar.value)) {
    const string raw{strValue};
    char *parseEnd = nullptr;
    const float parsedValue = std::strtof(raw.c_str(), &parseEnd);

    if (parseEnd != raw.c_str() + raw.size()) {
      return FeErr{Error("Invalid float32 value for KVar: " + raw, ErrorType::NullptrException)};
    }

    outVar.value = parsedValue;
  } else if (std::holds_alternative<string>(outVar.value)) {
    outVar.value = string{strValue};
  } else {
    return FeErr{Error("unsupported KVar type", ErrorType::SystemError)};
  }

  return {};
}

} // namespace flatearth
