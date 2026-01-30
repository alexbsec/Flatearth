#include "Filesystem.hpp"

#if FEPLATFORM_WINDOWS

#include "Core/Logger.hpp"

namespace flatearth::platform {

stdfs::path WorkDirectory() {
	return stdfs::current_path();
}



}

#endif