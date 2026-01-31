#include "Filesystem.hpp"

#if FEPLATFORM_WINDOWS

#include "Core/Logger.hpp"
#include <Windows.h>
#include <span>

namespace flatearth::platform {

stdfs::path WorkDirectory() {
	return stdfs::current_path();
}

static HANDLE GetHandle(FileHandle& handle) {
	return reinterpret_cast<HANDLE>(handle.nativeHandle);
}

FileSystem::FileSystem(memory::MemoryManager& memManager)
	: _memoryManager(memManager), _rootDir(WorkDirectory()) {}

bool FileSystem::Exists(const stdfs::path& path) const {
	return stdfs::exists(_rootDir / path);
}

uint64 FileSystem::SizeOfFile(const stdfs::path& path) const {
	std::error_code ec;
	const auto sz = stdfs::file_size(_rootDir / path, ec);
	if (ec) {
		FLOG_ERROR("file_size failed (ec={})", ec.value());
		return 0;
	}
	return static_cast<uint64>(sz);
}

FeExpect<FileHandle, Error> FileSystem::OpenFile(const stdfs::path& path, FileMode mode, bool binary) {
	(void)binary;

	// Desired access
	DWORD access = 0;
	const bool cCanRead = HasFileMode(mode, FileMode::Read);
	const bool cCanWrite = HasFileMode(mode, FileMode::Write);

	if (!cCanRead && !cCanWrite) {
		return FeErr{ Error("invalid file mode", ErrorType::InvalidFileMode) };
	}

	DWORD creation;
	if (cCanRead && cCanWrite) {
		access |= GENERIC_READ | GENERIC_WRITE;
		creation = OPEN_ALWAYS;
	}
	else if (cCanWrite) {
		access |= GENERIC_WRITE;
		creation = CREATE_ALWAYS;
	}
	else {
		access |= GENERIC_READ;
		creation = OPEN_EXISTING;
	}

	DWORD share = FILE_SHARE_READ;

	stdfs::path absolutePath = _rootDir / path;

	HANDLE handle = CreateFileW(
		absolutePath.c_str(),
		access,
		share,
		nullptr,
		creation,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (handle == INVALID_HANDLE_VALUE) {
		FLOG_ERROR("could not get file handle in windows API");
		return FeErr{Error("failed to open file", ErrorType::FileOpenError)};
	}

	return FileHandle{
		.nativeHandle = reinterpret_cast<void *>(handle),
		.valid = FeTrue,
	};
}

FeExpect<void, Error> FileSystem::CloseFile(FileHandle& handle) {
	if (!handle.valid || handle.nativeHandle == nullptr) {
		FLOG_ERROR("cannot close invalid file handle");
		return FeErr{ Error("invalid file handle", ErrorType::InvalidFileHandle) };
	}

	HANDLE winHandle = GetHandle(handle);
	if (winHandle == INVALID_HANDLE_VALUE) {
		FLOG_ERROR("could not get a valid windows handle");
		return FeErr{ Error("invalid file handle", ErrorType::InvalidFileHandle) };
	}

	if (!CloseHandle(winHandle)) {
		FLOG_ERROR("failed to close windows handle");
		return FeErr{ Error("failed to close file", ErrorType::FileCloseError) };
	}

	handle.valid = FeFalse;
	handle.nativeHandle = nullptr;
	return {};
}

FeExpect<uint64, Error> FileSystem::ReadFromFile(FileHandle& handle, std::span<std::byte> out) {
	if (!handle.valid || handle.nativeHandle == nullptr) {
		FLOG_ERROR("could not proceed becaus file handle is not valid or is nullptr");
		return FeErr{ Error("invalid file handle", ErrorType::InvalidFileHandle) };
	}

	HANDLE winHandle = GetHandle(handle);
	if (winHandle == INVALID_HANDLE_VALUE) {
		FLOG_ERROR("file to read is INVALID_HANDLE_VALUE state");
		return FeErr{ Error("invalid file handle", ErrorType::InvalidFileHandle) };
	}

	const uint64 cTotalSize = static_cast<uint64>(out.size());
	uint64 offset = 0;

	while (offset < cTotalSize) {
		const uint64 cChunkSize = (cTotalSize - offset) > static_cast<uint64>(MAXDWORD) ? static_cast<uint64>(MAXDWORD) : (cTotalSize - offset);
		const DWORD cChunk = static_cast<DWORD>(cChunkSize);

		DWORD bytesRead = 0;
		BOOL ok = ReadFile(
			winHandle,
			reinterpret_cast<void*>(out.data() + offset),
			cChunk,
			&bytesRead,
			nullptr
		);

		if (!ok) {
			FLOG_ERROR("could not read chunk from file");
			return FeErr{ Error("failed to read chunk from file", ErrorType::FileOpenError) };
		}

		if (bytesRead == 0) {
			break;
		}

		offset += static_cast<uint64>(bytesRead);

		if (bytesRead < cChunk) {
			break;
		}
	}

	return offset;
}

FeExpect<uint64, Error> FileSystem::WriteToFile(FileHandle& handle, std::span<const std::byte> data) {
	if (!handle.valid || handle.nativeHandle == nullptr) {
		FLOG_ERROR("could not proceed becaus file handle is not valid or is nullptr");
		return FeErr{ Error("invalid file handle", ErrorType::InvalidFileHandle) };
	}

	HANDLE winHandle = GetHandle(handle);
	if (winHandle == INVALID_HANDLE_VALUE) {
		FLOG_ERROR("file to read is INVALID_HANDLE_VALUE state");
		return FeErr{ Error("invalid file handle", ErrorType::InvalidFileHandle) };
	}

	const uint64 cTotalSize = static_cast<uint64>(data.size());
	uint64 offset = 0;

	while (offset < cTotalSize) {
		const uint64 cChunkSize = (cTotalSize - offset) > static_cast<uint64>(MAXDWORD) ? static_cast<uint64>(MAXDWORD) : (cTotalSize - offset);
		const DWORD cChunk = static_cast<DWORD>(cChunkSize);

		DWORD bytesWritten = 0;
		BOOL ok = WriteFile(winHandle,
			reinterpret_cast<const void*>(data.data() + offset),
			cChunk,
			&bytesWritten,
			nullptr);

		if (!ok) {
			return FeErr{ Error("failed to write to file", ErrorType::FileOpenError) };
		}

		offset += static_cast<uint64>(bytesWritten);

		if (bytesWritten == 0) {
			break;
		}
	}

	return offset;
}

void FileSystem::SetRootDirectory(const stdfs::path& path) {
	_rootDir = path;
}

}

#endif