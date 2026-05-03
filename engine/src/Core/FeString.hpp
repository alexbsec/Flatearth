#ifndef _FLATEARTH_ENGINE_CORE_FE_STRING_HPP
#define _FLATEARTH_ENGINE_CORE_FE_STRING_HPP

#include "Defines.hpp"
#include "Error.hpp"

#include <cstdlib>
#include <cstring>

namespace flatearth {

template <std::size_t N>
class FeString {
public:
  FeExpect<void, Error> Set(stringv src) {
    if (src.size() >= N) {
      return FeErr{Error("string is too long to be saved in this fixed size FeString",
                         ErrorType::SizeOverflow)};
    }
    std::memcpy(_data, src.data(), src.size());
    _data[src.size()] = '\0';
    if (src.size() + 1 < N) {
      std::memset(_data + src.size() + 1, 0, N - src.size() - 1);
    }
    return {};
  }

  stringv View() const { return stringv(_data, strnlen(_data, N)); }

  string String() const { return string(View()); }

private:
  char _data[N]{};
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_FE_STRING_HPP
