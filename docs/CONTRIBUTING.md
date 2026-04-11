# Contributing to Flatearth Engine

Thanks for your interest in contributing. This document covers everything you need to get started.

---

## Requirements

- Linux (primary supported platform)
- Vulkan SDK — follow the [official setup guide](https://vulkan-tutorial.com/Development_environment)
- GCC >= 14.2 or Clang >= 19.1
- CMake 3.29+
- X11/XCB libraries (see README for your distro)

---

## Build and run

```bash
make build   # compile engine, shaders, and copy assets
make run     # run with sanitizers enabled
make         # build + run
```

Output goes to `debug/bin/`.

---

## Project structure

```
engine/src/     — core engine library
testbed/src/    — test application built on top of the engine
assets/         — shaders (GLSL + compiled SPIR-V) and textures
docs/           — project documentation
```

---

## Core concepts

### Error handling

Functions that can fail return `FeExpect<T, Error>` instead of exceptions. Always check the result:

```cpp
auto res = someFunction();
if (!res.has_value()) {
    return FeErr{res.error()};
}
// success — use res.value()
```

Return `{}` from a `FeExpect<void, Error>` function on success.

### Memory

Use `MemoryManager` — never `new`, `delete`, `malloc`, or `free` directly:

```cpp
auto ptr = _memoryManager.Allocate<MyType>(memory::Tag::SomeTag, args...);
_memoryManager.RawFree(ptr, size, memory::Tag::SomeTag);
```

### Logging

```cpp
FLOG_INFO("message {}", value);
FLOG_WARN("warning");
FLOG_ERROR("error: {}", value);
FLOG_DEBUG("debug");
```

---

## Code style

- Types and functions: `PascalCase`
- Member variables: `_camelCase`
- Constants: `cCamelCase`
- Namespaces: `flatearth::subsystem`
- Indent: 2 spaces for C++/GLSL, 4 for everything else (see `.editorconfig`)
- Line length: 80 characters
- No exceptions, no raw memory allocation, no `#pragma once`

---

## Workflow

1. Branch off `main` using the naming convention below
2. Make focused, single-purpose commits
3. Run `make` and confirm no errors or sanitizer violations
4. Open a pull request with a clear description of what and why

### Branch naming

```
feat/short-description
fix/short-description
docs/short-description
refactor/short-description
```

---

## Questions

Open a GitHub issue or ask the maintainer.
