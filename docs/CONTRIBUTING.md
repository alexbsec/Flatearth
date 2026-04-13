# Contributing to Flatearth Engine

Hey, glad you're here. This document should get you up and running and give you enough context to not feel lost in the codebase. Read it before writing any code — it'll save you time.

---

## Requirements

### General

- Vulkan SDK — follow the [official setup guide](https://vulkan-tutorial.com/Development_environment)
- GCC >= 14.2 or Clang >= 19.1 (Linux) / MSVC >= 19.42 (Windows)
- CMake 3.29+

### Linux

You'll need X11 and XCB for windowing. Install them for your distro:

**Ubuntu/Debian**
```bash
sudo apt install -y libx11-dev libxcb1-dev libxcb-keysyms1-dev libxcb-icccm4-dev \
libxcb-image0-dev libxcb-shm0-dev libxcb-xfixes0-dev libxcb-randr0-dev \
libxcb-render-util0-dev libxcb-xinerama0-dev libxcb-glx0-dev libx11-xcb-dev
```

**Fedora/RHEL**
```bash
sudo dnf install -y libX11-devel libxcb-devel xcb-util-keysyms-devel \
xcb-util-devel xcb-util-image-devel xcb-util-wm-devel xcb-util-renderutil-devel
```

**Arch**
```bash
sudo pacman -Syu xorg-server xorg-xrandr libx11 libxcb xcb-util xcb-util-wm xcb-util-image
```

### Windows

Install the latest MSVC with the C++ Desktop Development workload from Visual Studio, plus the Vulkan SDK from [lunarg.com](https://vulkan.lunarg.com/sdk/home).

---

## Building

### Linux

```bash
make build   # compiles engine + testbed, compiles shaders, copies assets
make run     # runs the testbed binary with ASAN/LSAN enabled
make         # build + run in one step
```

Binaries go to `debug/bin/`.

### Windows

```bat
cmake -S . -B build -G "Visual Studio 17 2022"
```

Open the generated solution in Visual Studio and build from there. Binaries end up in the `build/` directory.

---

## Project structure

It helps to know what lives where before you start digging:

```
engine/src/
  Core/        — memory manager, app loop, events, input, logger
  Math/        — Vec2D, Vec3D, Mat4D and math utilities
  Platform/    — windowing, filesystem, input (Linux + Windows)
  Renderer/    — frontend/backend renderer split
    Vulkan/    — all Vulkan-specific code, never touch this from outside
      Shaders/ — shader management
  Resources/   — resource types, texture loader
  Containers/  — DArray, HashMap, HashSet, LinkedList, Queue
  Vendor/      — third-party headers (stb_image, etc.)
testbed/src/   — test application that runs on top of the engine
assets/        — GLSL shaders + compiled SPIR-V, textures
docs/          — you are here
```

---

## Things you need to understand before touching code

### Error handling — `FeExpect<T, Error>`

We don't use exceptions. Every function that can fail returns a result type. You must check it:

```cpp
auto res = someFunction();
if (!res.has_value()) {
    FLOG_ERROR("something went wrong: {}", res.error().message);
    return FeErr{res.error()};
}
// all good, use res.value()
```

If a function signature is `FeExpect<void, Error>`, just `return {}` on the happy path.

### Memory — always use `MemoryManager`

Never use `new`, `delete`, `malloc`, or `free`. Everything goes through the engine's memory manager:

```cpp
// typed allocation (calls constructor)
auto ptr = _memoryManager.Allocate<MyType>(memory::Tag::SomeTag, constructorArgs...);

// raw allocation (for buffers, arrays)
void *p = _memoryManager.RawAlloc(size, alignment, memory::Tag::SomeTag);

// free raw allocation
_memoryManager.RawFree(p, size, memory::Tag::SomeTag);
```

The `Tag` enum exists so we can track how much memory each subsystem is using. Pick the right tag — if nothing fits, check the existing ones in `FeMemory.hpp` before adding a new one.

### Renderer frontend vs backend

The renderer is split into two layers on purpose:

- `FrontendRenderer` — the public-facing API. Platform and graphics API agnostic. This is what the engine talks to.
- `IRendererBackend` / `VulkanBackend` — the actual Vulkan implementation.

**Never call Vulkan directly from outside `Renderer/Vulkan/`.** If you need a new rendering feature, add it to the frontend interface first and implement it in the backend.

### Logging

```cpp
FLOG_INFO("renderer initialized in {}ms", elapsed);
FLOG_WARN("something looks off but we can recover");
FLOG_ERROR("this shouldn't happen: {}", res.error().message);
FLOG_FATAL("unrecoverable, engine will shut down");
FLOG_DEBUG("only visible in debug builds");
```

---

## Code style

Keep it consistent with what's already there. The main rules:

- Types, functions, methods: `PascalCase`
- Member variables: `_camelCase` (underscore prefix)
- Local constants: `cCamelCase`
- Namespaces: `flatearth::subsystem`
- Indent: 2 spaces for C++/GLSL, 4 for everything else
- Max line length: 80 characters
- Header guards: `#ifndef`, not `#pragma once`
- No exceptions, no raw memory allocation

If in doubt, look at how something similar is done nearby and match it.

---

## Submitting a PR

We don't give direct push access. Fork the repo, work in your fork, then open a PR back here.

1. Fork the repo on GitHub and clone your fork
2. Add the upstream remote so you can stay in sync:
   ```bash
   git remote add upstream https://github.com/alexbsec/FlatearthEngine.git
   git fetch upstream
   ```
3. Branch off `main` in your fork using the naming convention below
4. Do your work — one feature or fix per PR, don't bundle unrelated changes
5. Before opening the PR, sync with upstream to avoid conflicts:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```
6. Run `make` and make sure there are no errors and no ASAN/LSAN violations
7. Open a PR from your fork's branch to `alexbsec/FlatearthEngine:main`
8. Write a clear PR description: what you changed and why

### Branch naming

```
feat/short-description      — new feature
fix/short-description       — bug fix
docs/short-description      — docs only
refactor/short-description  — refactor, no behavior change
```

---

## Good first issues

Check the Issues tab on GitHub. Start with anything labeled `good first issue` — those are scoped to be self-contained with a clear definition of done. If nothing is labeled yet, ask @alexbsec.

---

## Questions

Open an issue or ask directly. This codebase does some non-trivial things (custom allocator, Vulkan renderer, result types) so it's normal to need some context at first. Don't spin your wheels for too long before asking.
