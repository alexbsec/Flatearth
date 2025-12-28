#ifndef _FLATEARTH_ENGINE_CLOCK_HPP
#define _FLATEARTH_ENGINE_CLOCK_HPP

#include "Defines.hpp"
#include <chrono>

namespace flatearth::clock {

static constexpr float64 sMillisecond = 0.000000001;

inline static float64 GetAbsoluteTime() {
  using clock = std::chrono::steady_clock;
  using seconds = std::chrono::duration<double>;

  return std::chrono::duration_cast<seconds>(
    clock::now().time_since_epoch()
  ).count();
}

class Clock {
public:
  void Start();
  void Update();
  void Stop();

  float64 Elapsed() const noexcept;

private:
  float64 _startTime{0.0};
  float64 _elapsed{0.0};
};

}

#endif // _FLATEARTH_ENGINE_CLOCK_HPP
