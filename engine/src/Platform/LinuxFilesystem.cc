#include "Filesystem.hpp"

#if FEPLATFORM_LINUX

#include "Core/Logger.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace flatearth::platform {

stdfs::path WorkDirectory() {
  return stdfs::current_path();
}

stdfs::path GetExecutableDirectory() {
  char buffer[4096];
  ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
  if (len <= 0) {
    return {};
  }

  buffer[len] = 0;
  return stdfs::path(buffer).parent_path();
}

stdfs::path ResolvePath(const stdfs::path &root, const stdfs::path &path) {
  if (path.is_absolute()) {
    return path;
  }

  return (root / path).lexically_normal();
}

int32 GetFileDescriptor(FileHandle &handle) {
  return static_cast<int32>(reinterpret_cast<intptr_t>(handle.nativeHandle));
}

FileSystem::FileSystem(memory::MemoryManager &memoryManager)
    : _memoryManager(memoryManager), _rootDir(GetExecutableDirectory()) {
}

bool FileSystem::Exists(const stdfs::path &path) const {
  stdfs::path absolutePath = ResolvePath(_rootDir, path);
  return stdfs::exists(absolutePath);
}

uint64 FileSystem::SizeOfFile(const stdfs::path &path) const {
  stdfs::path absolutePath = ResolvePath(_rootDir, path);
  return stdfs::file_size(absolutePath);
}

FeExpect<FileHandle, Error>
FileSystem::OpenFile(const stdfs::path &path, FileMode mode, bool binary) {
  int32 flags = 0;
  if (HasFileMode(mode, FileMode::Read) && HasFileMode(mode, FileMode::Write)) {
    flags = O_RDWR | O_CREAT;
  } else if (HasFileMode(mode, FileMode::Read)) {
    flags = O_RDONLY;
  } else if (HasFileMode(mode, FileMode::Write)) {
    flags = O_WRONLY | O_CREAT | O_TRUNC;
  } else {
    return FeErr{Error("invalid file mode", ErrorType::InvalidFileMode)};
  }

  stdfs::path absolutePath = ResolvePath(_rootDir, path);
  FLOG_INFO("absolute path is {}", absolutePath.string());

  int32 fd = open(absolutePath.c_str(), flags, S_IRUSR | S_IWUSR);
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

FeExpect<uint64, Error> FileSystem::ReadFromFile(FileHandle &handle, std::span<std::byte> out) {
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

void FileSystem::SetRootDirectory(const stdfs::path &path) {
  _rootDir = path;
}

} // namespace flatearth::platform

#endif // FEPLATFORM_LINUX
