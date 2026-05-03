# Flatearth Engine

Flatearth Engine is an open-source 2D game engine written in C++23. It was born from the observation that most open-source engines treat 2D as a second-class citizen of 3D pipelines. Flatearth is built from the ground up for 2D — no 3D baggage, no compromises.

---

## Current State

The engine is in active development. The core loop — ECS, rendering, UI, audio, physics, and asset management — is functional and used in the bundled testbed game.

| System | Status |
|---|---|
| ECS (entities, components, views, scheduler) | ✅ implemented |
| Vulkan renderer (game + UI passes) | ✅ implemented |
| UI (anchored quads, solid color, TTF text) | ✅ implemented |
| Transform hierarchy | ✅ implemented |
| Sprite & sprite animation | ✅ implemented |
| Tilemap loading (TMX) | ✅ implemented |
| Audio | ✅ implemented |
| Particle system | ✅ implemented |
| Physics (RigidBody, AABB, circle colliders) | ✅ implemented |
| Asset management (textures, materials, fonts) | ✅ implemented |
| Scene management | ✅ implemented |
| Input (keyboard + mouse) | ✅ implemented |
| KVars (runtime config registry) | ✅ implemented |
| Memory management (tagged allocator) | ✅ implemented |
| UI buttons (hit-testing, interaction) | 🔧 planned |
| Editor | 🔧 planned |
| Networking / multiplayer | 🔧 planned |
| Scripting / hot-reload | 🔧 planned |
| Windows support | 🔧 in progress |

---

## Platform Support

| Platform | Status |
|---|---|
| Linux (X11/XCB) | ✅ primary |
| Windows | 🔧 in progress |
| macOS | not planned |

---

## Tech Stack

- **Language:** C++23
- **Rendering:** Vulkan
- **UI font rasterization:** stb_truetype
- **Tilemap format:** TMX (Tiled)
- **Build system:** CMake 3.20+ + GNU Make

---

## Project Structure

```
FlatearthEngine/
├── engine/       # Core engine library (libflatearth.so)
├── testbed/      # Playable testbed demonstrating engine features
├── assets/       # Shared shaders (GLSL → SPIR-V)
├── debug/bin/    # Build output (binaries + assets)
└── Makefile      # Top-level build orchestration
```

### `engine/`
The shared library. Contains all subsystems — ECS, renderer, asset pipeline, audio, physics, input, and platform layer. Games link against it; they do not modify it.

### `testbed/`
A small top-down game demonstrating the engine API: player movement, animated sprites, tilemaps, particles, audio, camera follow, UI (HP bar, text), and KVar-driven tuning.

---

## Architecture Overview

### ECS

Entities are plain IDs. Components are trivially copyable POD structs stored in typed sparse sets. Systems iterate components via typed views with no virtual dispatch in the hot path.

```cpp
// Spawning an entity with components
ecs::EntityId player = registry.Spawn()
    .With(scene::Transform2D{0.0f, 0.0f})
    .With(scene::Sprite{...})
    .With(PlayerTag{})
    .OwnedBy(sceneId)
    .Commit();

// Iterating a view
for (auto [id, xform, sprite] : registry.ViewOf<Transform2D, Sprite>()) {
    // ...
}
```

Systems implement `ISystem` and are registered with ordering constraints:

```cpp
scheduler.Register<PlayerSystem>(ctx, level1)
    .or_fatal("failed")
    .Before<scene::systems::TransformSystem>();
```

### Rendering

Two render passes per frame:
1. **Game pass** — world-space objects sorted by layer and material, projected through the camera's orthographic view.
2. **UI pass** — identity projection, NDC coordinates, drawn on top.

`UIAnchor` positions elements in normalized [0, 1] screen space. `UIStyle` renders solid-color or textured quads. `UIText` renders TTF glyphs from a baked atlas with proper baseline alignment.

### Asset Management

All assets go through `AssetManager`, which wraps reference-counted caches:

```cpp
// Load a sprite
auto sprite = ctx.assets.Manager()
    .LoadSprite("assets/textures/hero.png", resources::MeshShape::Quad)
    .or_fatal("failed to load sprite");

// Load a font
auto fontHandle = ctx.assets.Manager()
    .LoadFont("ui_font", "assets/fonts/myfont.ttf", 48.0f, 512)
    .or_fatal("failed to load font");
```

### Scenes

Each scene owns a set of entities. Destroying a scene destroys all entities it owns.

```cpp
scene::SceneId level1 = ctx.project.RegisterScene("level1");

registry.Spawn()
    .With(MyComponent{})
    .OwnedBy(level1)  // entity is cleaned up when level1 is destroyed
    .Commit();
```

### UI Text

Text renders with correct baseline alignment across mixed-case glyphs. Glyph sizes are aspect-ratio corrected so text stays proportional on any window size.

```cpp
ui::UIText uiText{};
uiText.handle = fontHandle;
uiText.color  = {{1.0f, 1.0f, 0.0f}, 1.0f};  // yellow
uiText.text.Set("Hello Flatearth!");

registry.Spawn()
    .With(ui::UIAnchor{.normalizedX = 0.05f, .normalizedY = 0.93f,
                       .scaleX = 0.08f, .scaleY = 0.08f})
    .OwnedBy(sceneId)
    .Commit();
registry.Insert<ui::UIText>(entity, uiText);
```

### KVars

Runtime-tunable values exposed through the debug UI:

```cpp
ctx.core.KVarsRegistry()
    .Register("player.speed", "Movement speed (units/s)", 1.5f)
    .or_log_error("failed to register kvar");

float32 speed = ctx.core.KVarsRegistry()
    .Get<float32>("player.speed")
    .value_or(1.5f);
```

---

## Requirements

### All platforms
- CMake 3.20+
- C++23 compiler: Clang ≥ 19.1, GCC ≥ 14.2, or MSVC ≥ 19.42
- Vulkan SDK — see [vulkan-tutorial.com/Development_environment](https://vulkan-tutorial.com/Development_environment)

### Linux

Install X11/XCB windowing libraries:

**Ubuntu / Debian**
```bash
sudo apt install -y libx11-dev libxcb1-dev libxcb-keysyms1-dev libxcb-icccm4-dev \
    libxcb-image0-dev libxcb-shm0-dev libxcb-xfixes0-dev libxcb-randr0-dev \
    libxcb-render-util0-dev libxcb-xinerama0-dev libxcb-glx0-dev libx11-xcb-dev
```

**Fedora / RHEL**
```bash
sudo dnf install -y libX11-devel libxcb-devel xcb-util-keysyms-devel \
    xcb-util-devel xcb-util-image-devel xcb-util-wm-devel xcb-util-renderutil-devel
```

**Arch Linux**
```bash
sudo pacman -Syu xorg-server libx11 libxcb xcb-util xcb-util-wm xcb-util-image
```

### Windows

Install Visual Studio with the **Desktop development with C++** workload and the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home).

---

## Building

### Linux

```bash
# Build everything (engine + testbed)
./build-all.sh

# Or via Make
make build   # compile
make run     # compile and launch testbed
```

Output lands in `./debug/bin/`.

### Windows

```bat
cmake -S . -B build -G "Visual Studio 17 2022"
```

Open the generated solution in Visual Studio and build. Binaries go to `./build/`.

---

## Testbed Controls

| Key | Action |
|---|---|
| W / A / S / D | Move player |
| Arrow keys | Move player (alternative) |
| F | Deal 10 damage (HP bar test) |
| R | Restore full health |
| Mouse wheel | Zoom camera (if wired) |

---

## Contributing

The engine is under active development. Contributions, bug reports, and feedback are welcome via GitHub issues and pull requests.
See [Contributing Guide](docs/CONTRIBUTING.md)
