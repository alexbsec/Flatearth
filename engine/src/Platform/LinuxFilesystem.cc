#include "Filesystem.hpp"

#if FEPLATFORM_LINUX

#include <fcntl.h>
#include <unistd.h>

namespace flatearth::platform {

int32 GetFileDescriptor(FileHandle &handle) {
  return static_cast<int32>(reinterpret_cast<intptr_t>(handle.nativeHandle));
}

FileSystem::FileSystem(memory::MemoryManager &memoryManager)
    : _memoryManager(memoryManager) {}

bool FileSystem::Exists(const stdfs::path &path) const {
  return stdfs::exists(path);
}

uint64 FileSystem::SizeOfFile(const stdfs::path &path) const {
  return stdfs::file_size(path);
}

FeExpect<FileHandle, Error> FileSystem::OpenFile(const stdfs::path &path,
                                           FileMode mode,
                                           bool binary) {
  int32 flags = 0;
  if (HasFileMode(mode, FileMode::Read) &&
      HasFileMode(mode, FileMode::Write)) {
    flags = O_RDWR | O_CREAT;
  } else if (HasFileMode(mode, FileMode::Read)) {
    flags = O_RDONLY;
  } else if (HasFileMode(mode, FileMode::Write)) {
    flags = O_WRONLY | O_CREAT | O_TRUNC;
  } else {
    return FeErr{Error("invalid file mode", ErrorType::InvalidFileMode)};
  }
 
  int32 fd = open(path.c_str(), flags, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    return FeErr{Error("failed to open file", ErrorType::FileOpenError)};
  }

  return FileHandle{
    .nativeHandle = reinterpret_cast<void *>(static_cast<intptr_t>(fd)),
    .valid = FeTrue,
  };
}

FeExpect<void, Error> FileSystem::CloseFile(FileHandle &handle) {
  if (!handle.valid || handle.nativeHandle == nullptr) {
    return FeErr{Error("invalid file handle", ErrorType::InvalidFileHandle)};
  }

  int32 fd = GetFileDescriptor(handle);
  if (close(fd) < 0) {
    return FeErr{Error("failed to close file", ErrorType::FileCloseError)};
  }

  handle.valid = FeFalse;
  handle.nativeHandle = nullptr;
  return {};
}

FeExpect<uint64, Error> FileSystem::ReadFromFile(FileHandle &handle,
                                             std::span<std::byte> out) {
  if (!handle.valid || handle.nativeHandle == nullptr) {
    return FeErr{Error("invalid file handle", ErrorType::InvalidFileHandle)};
  }

  int32 fd = GetFileDescriptor(handle);
  ssize_t bytesRead = read(fd, out.data(), out.size());
  if (bytesRead < 0) {
    return FeErr{Error("failed to read from file", ErrorType::FileOpenError)};
  }

  return static_cast<uint64>(bytesRead);
}

FeExpect<uint64, Error> FileSystem::WriteToFile(FileHandle &handle,
                                              std::span<const std::byte> data) {
  if (!handle.valid || handle.nativeHandle == nullptr) {
    return FeErr{Error("invalid file handle", ErrorType::InvalidFileHandle)};
  }

  int32 fd = GetFileDescriptor(handle);
  ssize_t bytesWritten = write(fd, data.data(), data.size());
  if (bytesWritten < 0) {
    return FeErr{Error("failed to write to file", ErrorType::FileOpenError)};
  }

  return static_cast<uint64>(bytesWritten);
}

}

#endif // FEPLATFORM_LINUX
