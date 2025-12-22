#include "Clock.hpp"

namespace flatearth::clock {

void Clock::Start() {
  _startTime = GetAbsoluteTime();
}

void Clock::Update() {
  if (_startTime != 0) {
    _elapsed = GetAbsoluteTime() - _startTime;
  }
}

float64 Clock::Elapsed() const noexcept {
  return _elapsed;
}

}
