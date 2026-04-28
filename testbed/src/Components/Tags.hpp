#ifndef _FLATEARTH_TESTBED_COMPONENTS_TAGS_HPP
#define _FLATEARTH_TESTBED_COMPONENTS_TAGS_HPP

#include <Defines.hpp>

namespace flatearth::testbed {

struct PlayerTag {};
struct WorldTag {};
struct ParticleTag {};

struct PlayerMovementState {
  uint8 lastDirRow{0};
  uint8 vertRow{0};
  uint8 horzRow{1};
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTBED_COMPONENTS_TAGS_HPP
