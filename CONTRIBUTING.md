# Contributing to Flatearth Engine

Thanks for your interest in contributing. This document will help you get oriented before touching any code.

---

## Before you start

Flatearth is written in **C++23**. If you're coming from Go or TypeScript, the concepts are familiar (structs, interfaces, error handling, memory) but the syntax and tooling are different. Don't worry — the tasks listed in the issues are scoped to be approachable without deep C++ experience. Read this document carefully first and ask questions before writing code.

---

## Building the project

### Requirements

- Linux (primary platform — start here, not Windows)
- Vulkan SDK — follow [this guide](https://vulkan-tutorial.com/Development_environment)
- GCC >= 14.2 or Clang >= 19.1
- CMake 3.29+
- X11/XCB libraries (see README for your distro)

### Build and run

```bash
make build   # compiles the engine and testbed, compiles shaders, copies assets
make run     # runs the testbed binary with ASAN enabled
make         # build + run in one step
```

Binaries are output to `debug/bin/`.

---

## Project structure

```
engine/src/
  Core/        — memory manager, application loop, events, input, logger
  Math/        — Vec2D, Vec3D, Mat4D, math utilities
  Platform/    — window creation, filesystem, input (Linux + Windows)
  Renderer/    — frontend/backend renderer split
    Vulkan/    — all Vulkan-specific code lives here
      Shaders/ — shader management
  Resources/   — resource types, texture loader
  Containers/  — DArray, HashMap, HashSet, LinkedList, Queue
  Vendor/      — third-party headers (stb_image)
testbed/src/   — the game/test application that uses the engine
assets/        — shaders (GLSL source + compiled SPIR-V), textures
```

---

## Key concepts to understand first

### Result type — `FeExpect<T, Error>`

Instead of exceptions, every fallible function returns a result type. Always check it:

```cpp
auto res = someFunction();
if (!res.has_value()) {
    FLOG_ERROR("something failed: {}", res.error().message);
    return FeErr{res.error()};
}
// use res.value() or just res.value() is implicit in some cases
```

If a function returns `FeExpect<void, Error>`, return `{}` on success.

### Memory manager

Never use `new`/`delete` or `malloc`/`free`. Everything goes through `MemoryManager`:

```cpp
// allocate
auto ptr = _memoryManager.Allocate<MyType>(memory::Tag::SomeTag, constructorArgs...);

// raw allocation (for buffers)
void *p = _memoryManager.RawAlloc(size, alignment, memory::Tag::SomeTag);

// free raw allocation
_memoryManager.RawFree(p, size, memory::Tag::SomeTag);
```

The `Tag` enum helps track memory usage by subsystem.

### Renderer split

- `RendererFrontend` — the public API the engine uses. Platform agnostic.
- `IRendererBackend` — interface implemented by `VulkanBackend`.
- **Never call Vulkan directly from outside `Renderer/Vulkan/`.**

### Logging

```cpp
FLOG_INFO("message {}", variable);
FLOG_WARN("warning");
FLOG_ERROR("error: {}", someValue);
FLOG_FATAL("unrecoverable");
FLOG_DEBUG("debug only");
```

---

## Code style

- Class names: `PascalCase`
- Member variables: `_camelCase` (prefixed with `_`)
- Free functions and methods: `PascalCase`
- Constants: `cCamelCase` (prefixed with `c`)
- Namespaces: `flatearth::subsystem`
- No raw `new`/`delete`
- No exceptions — use `FeExpect`
- Headers use `#ifndef` guards, not `#pragma once` (check existing files)
- Keep includes sorted and minimal

---

## Branch naming

```
feat/short-description     — new feature
fix/short-description      — bug fix
docs/short-description     — documentation only
refactor/short-description — refactor, no behavior change
```

---

## Submitting a PR

1. Branch off `main`
2. Keep the PR focused — one fix or one feature per PR
3. Make sure `make` passes without errors or ASAN/LSAN violations before submitting
4. Describe what you changed and why in the PR description

---

## Good first issues

Check the GitHub Issues tab. Issues labeled `good first issue` are scoped to be self-contained and well-defined. If nothing is labeled yet, ask @alexbsec which one to pick up.

---

## Questions

Open a GitHub issue or ask directly. There are no stupid questions — this codebase has custom memory management, a Vulkan renderer, and C++23 features, so getting oriented takes time.
