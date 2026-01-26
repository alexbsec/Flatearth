#ifndef _FLATEARTH_ENGINE_FILESYSTEM_HPP
#define _FLATEARTH_ENGINE_FILESYSTEM_HPP

#include "Core/FeMemory.hpp"
#include "Defines.hpp"
#include "Error.hpp"
#include <filesystem>

namespace flatearth::platform {

namespace stdfs = std::filesystem;

enum class FileMode : uint8 {
  Read = 1 << 0,
  Write = 1 << 1,
};

FEAPI inline constexpr FileMode operator|(FileMode a, FileMode b) {
  return static_cast<FileMode>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

FEAPI inline constexpr bool HasFileMode(FileMode modes, FileMode mode) {
  return (static_cast<uint8>(modes) & static_cast<uint8>(mode)) != 0;
}

struct FileHandle {
  void *nativeHandle{nullptr};
  bool valid{false};

  explicit inline operator bool() const { return valid; }
};

class FileSystem {
public:
  FEAPI explicit FileSystem(memory::MemoryManager &memoryManager);
  FEAPI ~FileSystem() = default;

  FEAPI bool Exists(const stdfs::path &path) const;
  FEAPI uint64 SizeOfFile(const stdfs::path &path) const;

  FEAPI FeExpect<FileHandle, Error>
  OpenFile(const stdfs::path &path, FileMode mode, bool binary = FeTrue);

  FEAPI FeExpect<void, Error> CloseFile(FileHandle &handle);

  FEAPI FeExpect<uint64, Error> ReadFromFile(FileHandle &handle,
                                             std::span<std::byte> out);
  FEAPI FeExpect<uint64, Error> WriteToFile(FileHandle &handle,
                                            std::span<const std::byte> data);

  FEAPI void SetRootDirectory(const stdfs::path &path);

private:
  memory::MemoryManager &_memoryManager;
  stdfs::path _rootDir{};
};

} // namespace flatearth::platform

#endif // _FLATEARTH_ENGINE_FILESYSTEM_HPP
