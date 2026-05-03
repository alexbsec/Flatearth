#ifndef _FLATEARTH_ENGINE_CORE_KVAR_REGISTRY_HPP
#define _FLATEARTH_ENGINE_CORE_KVAR_REGISTRY_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "Defines.hpp"

#include <variant>

namespace flatearth {

using KVarValue = std::variant<bool, int32, float32, string>;

struct KVar {
  string name{};
  string description{};
  KVarValue value{};
  KVarValue defaultValue{};
  std::function<void(const KVarValue &)> onChange{nullptr};
};

class KVarRegistry {
public:
  explicit KVarRegistry(memory::MemoryManager &memManager);

  FeExpect<void, Error> Register(stringv name, stringv desc, KVarValue);
  FeExpect<bool, Error> ParseAndSet(stringv name, stringv strValue);
  string GetAsString(stringv name) const;

public:
  template <typename T>
  FeExpect<T, Error> Get(stringv name) const {
    const string key{name};

    if (!_localMap.Has(key)) {
      string errMsg = std::format("KVar not found for key '{}'", key);
      return FeErr{Error(errMsg, ErrorType::SystemError)};
    }

    const KVar *pKvar = _localMap.Retrieve(key);
    if (pKvar == nullptr) {
      return _cRetrieveErr;
    }

    if (!std::holds_alternative<T>(pKvar->value)) {
      return FeErr{Error("KVar type mismatch", ErrorType::SystemError)};
    }

    return std::get<T>(pKvar->value);
  }

  template <typename T>
  FeExpect<void, Error> Set(stringv name, T value) {
    const string key{name};

    if (!_localMap.Has(key)) {
      return {};
    }

    KVar *pKvar = _localMap.Retrieve(key);
    if (pKvar == nullptr) {
      return _cRetrieveErr;
    }

    if (!std::holds_alternative<T>(pKvar->value)) {
      return FeErr{Error("KVar type mismatch", ErrorType::SystemError)};
    }

    pKvar->value = value;

    if (pKvar->onChange != nullptr) {
      pKvar->onChange(pKvar->value);
    }

    return {};
  }

  template <typename Fn>
  void ForEach(Fn &&fn) {
    _localMap.ForEach(std::forward<Fn>(fn));
  }

private:
  FeExpect<void, Error> ParseKVarValue(stringv strValue, KVar &outVar);

private:
  const std::unexpected<Error> _cRetrieveErr{
      Error("KVar retrieval failed", ErrorType::NullptrException)};

private:
  containers::HashMap<string, KVar> _localMap;
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_KVAR_REGISTRY_HPP
