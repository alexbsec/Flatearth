#ifndef _FLATEARTH_ENGINE_CLOCK_HPP
#define _FLATEARTH_ENGINE_CLOCK_HPP

#include "Defines.hpp"
#include <ctime>

namespace flatearth::clock {

static constexpr float64 sMillisecond = 0.000000001;

inline static float64 GetAbsoluteTime() {
  struct std::timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec + now.tv_nsec * sMillisecond;
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
