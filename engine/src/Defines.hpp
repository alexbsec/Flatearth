#ifndef _FLATEARTH_ENGINE_DEFINITIONS_HPP
#define _FLATEARTH_ENGINE_DEFINITIONS_HPP

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <source_location>
#include <sstream>
#include <string>

// String alias
using string = std::string;
using stringv = std::string_view;
using osstream = std::ostringstream;

// Unsigned types
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

// Signed types
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

// Floating point types
using float32 = float;
using float64 = double;

using atomic_bool = std::atomic<bool>;

#if defined(_MSVC_LANG)
#define STATIC_ASSERT static_assert
#elif defined(__cplusplus) && __cplusplus >= 201703L
#define STATIC_ASSERT static_assert
#else
#error "C++17 or newer needed to proceed"
#endif

// Ensure all types are of the correct size
STATIC_ASSERT(sizeof(uint8) == 1, "Expected uint8 to be 1 byte");
STATIC_ASSERT(sizeof(uint16) == 2, "Expected uin16 to be 2 bytes");
STATIC_ASSERT(sizeof(uint32) == 4, "Expected uint32 to be 4 bytes");
STATIC_ASSERT(sizeof(uint64) == 8, "Expected uint64 to be 8 bytes");

STATIC_ASSERT(sizeof(int8) == 1, "Expected int8 to be 1 byte");
STATIC_ASSERT(sizeof(int16) == 2, "Expected int16 to be 2 bytes");
STATIC_ASSERT(sizeof(int32) == 4, "Expected int32 to be 4 bytes");
STATIC_ASSERT(sizeof(int64) == 8, "Expected int64 to be 8 bytes");

STATIC_ASSERT(sizeof(float32) == 4, "Expected float32 to be 4 bytes");
STATIC_ASSERT(sizeof(float64) == 8, "Expected float64 to be 8 bytes");

#define FeTrue true
#define FeFalse false

namespace flatearth::detail {
[[noreturn]] void FatalLog(stringv userMsg, stringv errMsg);
void ErrorLog(stringv userMsg, stringv errMsg, std::source_location loc);
void WarnLog(stringv userMsg, stringv errMsg, std::source_location loc);
} // namespace flatearth::detail

template <typename... Args>
struct FmtWithLoc {
  stringv fmt;
  std::source_location loc;
  constexpr FmtWithLoc(const char *f, std::source_location l = std::source_location::current())
      : fmt(f), loc(l) {}
  constexpr FmtWithLoc(stringv f, std::source_location l = std::source_location::current())
      : fmt(f), loc(l) {}
};

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define FEPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required for windows"
#endif // _WIN64
#elif defined(__linux__) || defined(__gnu_linux__)
#define FEPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define FEPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
#define FEPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
#define FEPLATFORM_POSIX 1
#else
#error "Unknown platform"
#endif

#ifdef FEXPORT
// Exports
#ifdef _MSC_VER
#define FEAPI __declspec(dllexport)
#else
#define FEAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define FEAPI __declspec(dllimport)
#else
#define FEAPI
#endif
#endif

#ifndef _DEBUG
// Set to false if in release
#define _DEBUG FeTrue
#endif

#define FCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;

// Inlining
#ifdef _MSC_VER
#define FEINLINE static __forceinline
#define FENOINLINE static __declspec(noinline)
#else
#define FEINLINE static inline
#define FENOINLINE
#endif

constexpr static float64 FE_PI = 3.14159265358979323846f;
constexpr static float64 FE_2_PI = 2 * FE_PI;
constexpr static float64 FE_HALF_PI = FE_PI / 2.0f;
constexpr static float64 FE_QUARTER_PI = FE_PI / 4.0f;
constexpr static float64 FE_1_OVER_PI = 1 / FE_PI;
constexpr static float64 FE_1_OVER_2_PI = 1 / FE_2_PI;
constexpr static float64 FE_SQRT_2 = 1.41421356237309504880f;
constexpr static float64 FE_SQRT_3 = 1.73205080756887729352f;
constexpr static float64 FE_1_OVER_SQRT_2 = 0.70710678118654752440f;
constexpr static float64 FE_1_OVER_SQRT_3 = 0.57735026918962576450f;
constexpr static float64 FE_DEG_TO_RAD_MUL = FE_PI / 180.0f;
constexpr static float64 FE_RAD_TO_DEG_MUL = 180.0f / FE_PI;

constexpr static float64 FE_MS_TO_SEC_MUL = 1 / 1000.0f;
constexpr static float64 FE_SEC_TO_MS_MUL = 1000.0f;

constexpr static float64 FE_F64MAX = 1e30f;
constexpr static float64 FE_F64EPS = 1.192092896e-7f;
constexpr static float32 FE_F32MAX = 1e20f;
constexpr static float32 FE_F32EPS = 1.192092896e-7f;

template <typename T>
using FePtr = std::unique_ptr<T, std::function<void(T *)>>;

template <typename T>
using FeSharedPtr = std::shared_ptr<T>;

template <typename T, typename D>
using FeCustomDeleterPtr = std::unique_ptr<T, D>;

template <typename T>
using FeOptional = std::optional<T>;

template <typename Ret, typename Err>
struct FeExpect : std::expected<Ret, Err> {
  using Base = std::expected<Ret, Err>;
  using Base::Base;

  auto or_fatal(stringv msg, std::source_location src = std::source_location::current())
    requires requires(Err e) { e.message; }
  {
    if (!this->has_value()) {
      flatearth::detail::FatalLog(msg, this->error().message);
    }
    if constexpr (!std::is_void_v<Ret>) {
      return std::move(this->value());
    }
  }

  template <typename... Args>
  [[nodiscard]] FeExpect or_error(FmtWithLoc<std::type_identity_t<Args>...> fmtloc, Args &&...args)
    requires requires(Err e) { e.message; }
  {
    if (!this->has_value()) {
      flatearth::detail::ErrorLog(std::vformat(fmtloc.fmt, std::make_format_args(args...)),
                                  this->error().message,
                                  fmtloc.loc);
    }
    return std::move(*this);
  }

  template <typename... Args>
  void or_log_error(FmtWithLoc<std::type_identity_t<Args>...> fmtloc, Args &&...args)
    requires requires(Err e) { e.message; }
  {
    if (!this->has_value()) {
      flatearth::detail::ErrorLog(std::vformat(fmtloc.fmt, std::make_format_args(args...)),
                                  this->error().message,
                                  fmtloc.loc);
    }
  }

  template <typename... Args>
  void or_log_warn(FmtWithLoc<std::type_identity_t<Args>...> fmtloc, Args &&...args)
    requires requires(Err e) { e.message; }
  {
    if (!this->has_value()) {
      flatearth::detail::WarnLog(std::vformat(fmtloc.fmt, std::make_format_args(args...)),
                                 this->error().message,
                                 fmtloc.loc);
    } else {
      flatearth::detail::WarnLog(
          std::vformat(fmtloc.fmt, std::make_format_args(args...)), "", fmtloc.loc);
    }
  }

  bool errored() const
    requires requires(Err e) { e.message; }
  {
    return !this->has_value();
  }

  template <typename Fn>
  auto and_then(Fn &&fn) && {
    if constexpr (std::is_void_v<Ret>) {
      using ResultType = std::invoke_result_t<Fn>;
      if (this->has_value())
        return std::invoke(std::forward<Fn>(fn));
      return ResultType(std::unexpect, std::move(this->error()));
    } else {
      using ResultType = std::invoke_result_t<Fn, Ret &&>;
      if (this->has_value())
        return std::invoke(std::forward<Fn>(fn), std::move(this->value()));
      return ResultType(std::unexpect, std::move(this->error()));
    }
  }

  template <typename Fn>
  auto and_then(Fn &&fn) const & {
    if constexpr (std::is_void_v<Ret>) {
      using ResultType = std::invoke_result_t<Fn>;
      if (this->has_value())
        return std::invoke(std::forward<Fn>(fn));
      return ResultType(std::unexpect, this->error());
    } else {
      using ResultType = std::invoke_result_t<Fn, const Ret &>;
      if (this->has_value())
        return std::invoke(std::forward<Fn>(fn), this->value());
      return ResultType(std::unexpect, this->error());
    }
  }
};

template <typename Err>
using FeErr = std::unexpected<Err>;

template <typename T>
inline T *FeCast(void *ptr) {
  static_assert(!std::is_array_v<T>, "FeCast<T>: T must not be an array type");
#ifdef FE_DEBUG
  assert(ptr != nullptr);
#endif
  return static_cast<T *>(ptr);
}

template <typename T>
inline T *FeCastPermissive(void *ptr) {
#ifdef FE_DEBUG
  assert(ptr != nullptr);
#endif
  return reinterpret_cast<T *>(ptr);
}

#define FECLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;

#define FE_RIPPLE(expr)                                          \
  do {                                                           \
    auto _fe_ripple_tmp = (expr);                                \
    if (!_fe_ripple_tmp.has_value())                             \
      return std::unexpected(std::move(_fe_ripple_tmp.error())); \
  } while (0)

#define FE_MOVE_ONLY(Type)                     \
  Type(Type &&) noexcept = default;            \
  Type &operator=(Type &&) noexcept = default; \
  Type(const Type &) = delete;                 \
  Type &operator=(const Type &) = delete;

#define FE_MOVE_CONSTRUCT_ONLY(Type) \
  Type(Type &&) noexcept = default;  \
  Type &operator=(Type &&) = delete; \
  FE_NO_COPY(Type)

#endif // _FLATEARTH_ENGINE_DEFINITIONS_HP
