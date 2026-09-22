# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

OpenEngine is a minimal 3D engine/game built in C++17 on OpenGL (GLEW + GLFW + GLM), for Windows/MSVC. Physics and collision are handled by [Jolt Physics](https://github.com/jrouwe/JoltPhysics). There is no engine editor or scene file format — game objects are constructed and wired up directly in `src/main.cpp`.

## Build

The project uses CMake with Ninja as the generator, building with MSVC (cl.exe) on Windows.

```powershell
# Jolt is a git submodule — required after a fresh clone
git submodule update --init --recursive

# configure (first time) — presets live in CMakePresets.json
cmake --preset debug      # -> build/          (Debug)
cmake --preset release    # -> build-release/  (Release)

# build
cmake --build --preset debug
cmake --build --preset release

# run from inside the build folder, so the relative shader paths resolve — see Shaders below
cd build; .\OpenEngine.exe
```

Both folders are single-config Ninja: **the folder decides the build type** (`--config` is ignored). VS Code's CMake Tools uses the same presets — kits are disabled when `CMakePresets.json` exists; pick the configure/build preset in the CMake panel or status bar. `cl.exe` comes from the VS developer environment (vcvars64); the `architecture`/`toolset` entries with `"strategy": "external"` tell CMake Tools to set that up. Do not add `CMAKE_C/CXX_COMPILER` to the presets: a value that differs textually from the cached full path makes CMake wipe the cache.

There are no configured lint or test targets/frameworks in this project (no test runner, no `ctest` targets).

**Header changes are not tracked by ninja in this setup** (the MSVC `/showIncludes` prefix is localized to Korean and doesn't match `msvc_deps_prefix`). After editing a header — especially one that changes a class layout, like `gameObject.h` — delete the engine's objects so every TU recompiles: remove `<build folder>\CMakeFiles\OpenEngine.dir\src\*.obj` (by explicit path) in **each** build folder you use. Avoid `--clean-first`, which also rebuilds all of Jolt (several minutes).

Files containing Korean comments must be saved as **UTF-8 with BOM**; `/utf-8` does not reach the generated build, so MSVC otherwise reads them as CP949 (warning C4819, and a comment can swallow the next line).

### Jolt integration (`CMakeLists.txt`)

Jolt is added via `add_subdirectory(dependencies/JoltPhysics/Build)` and linked as the `Jolt` target, which exports its include path and `JPH_*` defines as PUBLIC — never add `JPH_*` defines by hand (a mismatch aborts at `RegisterTypes()`). Three options must be set before `add_subdirectory` or linking fails with LNK2038:
- `USE_STATIC_MSVC_RUNTIME_LIBRARY OFF` — match the dynamic runtime (`/MD`) used by the engine and GLFW.
- `OVERRIDE_CXX_FLAGS OFF` — otherwise Jolt replaces `CMAKE_CXX_FLAGS_DEBUG/RELEASE`, dropping `/MDd` so Jolt compiles with the default `/MT`.
- `CPP_RTTI_ENABLED ON` — MSVC defaults to `/GR`; Jolt defaults to `/GR-`.

## Architecture

### Object model

`GameObject` (`include/gameObject.h`) is the base class for everything placed in the world: `position`, `rotation` (quaternion), a `Collider*` shape description, `isStatic`, the Jolt body-creation parameters (`useGravity`, `mass`, `friction`), and a `JPH::BodyID bodyID`. `update(dt)` is an empty per-object game-logic hook; movement and collision are Jolt's job.

- `Collider` / `BoxCollider` (`include/collider.h`, `include/boxCollider.h`) are **shape descriptions only** (type, `offset`, `size`) — they contain no collision logic. `ColliderType::Sphere` exists but `sphereCollider.h` is an empty stub and `PhysicsWorld::addBody` only supports boxes.
- `Cube` / `Plane` (`include/cube.h`, `include/plane.h` + `.cpp`) derive from `RenderableObject` (owns VAO/VBO, `draw()`), build interleaved position+color vertex data, and default to `isStatic = true`. `Plane::scale` resizes the `BoxCollider` to match the visual scale; the plane's collider is 1 unit thick with `offset.y = -0.5` so its top face sits at the rendered plane.
- `RenderableObject::draw()` builds `translate(position) * mat4_cast(rotation) * localMatrix`; `localMatrix` holds scale only.
- `Player` (`include/player.h`, `src/player.cpp`) is a `JPH::CharacterVirtual` (capsule, radius 0.3, height 1.0) whose origin is the feet; the camera sits at `position + cameraOffset`. It is not in `Scene` and has no `Collider`.

### Physics (Jolt)

`PhysicsWorld` (`include/physicsWorld.h`, `src/physicsWorld.cpp`) wraps Jolt:
- Two object layers (`Layers::NON_MOVING`, `Layers::MOVING`) mapped 1:1 to broad-phase layers; static-vs-static pairs never collide. The three required layer interfaces live in the header. `GetBroadPhaseLayerName` must be implemented because the profiler is on in Debug/Release.
- `init()` order is fixed: `RegisterDefaultAllocator` → `Factory` → `RegisterTypes` → temp allocator → job system → `PhysicsSystem` (held by pointer so it's constructed after the allocator is registered). `shutdown()` reverses it.
- `update(dt)` accumulates frame time and steps Jolt at a fixed 60 Hz (clamped to 0.25 s of backlog).
- `addBody(const GameObject&)` turns a `BoxCollider` into a `BoxShape` (**half extents** = `size / 2`), wraps it in a `RotatedTranslatedShape` when `offset != 0` so the body origin always equals `GameObject::position`, and maps `isStatic` → Static/NON_MOVING vs Dynamic/MOVING, `useGravity` → gravity factor, `mass` → mass override, `friction` → friction.

`include/joltConversions.h` converts GLM ↔ Jolt. **Quaternion component order differs**: GLM's constructor is `(w, x, y, z)`, Jolt's is `(x, y, z, w)`.

`Scene` (`include/scene.h`) owns renderable objects and their bodies:
- `spawn<T>()` does **not** create a body. `updatePhysics(dt)` creates bodies lazily for objects that have a collider but no body, so settings applied after `spawn` (`isStatic`, `translate`, `rotate`, `scale`) are respected. After a body exists, changing those fields has no effect on Jolt.
- `updatePhysics` then steps `PhysicsWorld` and copies each dynamic body's transform back into `position`/`rotation`. `OptimizeBroadPhase` runs only when new static bodies were added.
- `destroy()` removes the object's body; `removeAllBodies()` must be called before `PhysicsWorld::shutdown()`.

`Player`:
- `createCharacter()` after `PhysicsWorld::init()`; `destroyCharacter()` before shutdown (the character owns an inner rigid body registered in the world — that inner body is what lets falling bodies collide with the player).
- Input only sets `setMoveInput()` / `jump()`; `updateCharacter(dt, world)` converts them to a velocity (following Jolt's `CharacterVirtualTest`), runs `ExtendedUpdate` (stair stepping, stick-to-floor, pushing bodies), and copies the result to `position`.

Test scenes: avoid perfectly symmetric drop orientations (e.g. 60° about the (1,1,1) diagonal) when checking tumbling — a box can come to rest balanced on an edge and Jolt puts it to sleep there.

### Rendering / main loop

`src/main.cpp` owns the GLFW window, input callbacks, and the frame loop. Per frame: `processInput` → `player.updateCharacter` → `scene.updatePhysics` → `scene.update` → view matrix from the player's camera → clear → upload MVP uniforms → `scene.draw()` → poll/swap. On exit: `player.destroyCharacter()` → `scene.removeAllBodies()` → `physicsWorld.shutdown()`.

`Shader` (`include/shader.h`, header-only) compiles/links `src/vShader.glsl` and `src/fShader.glsl` from source at startup; CMake copies both `.glsl` files into the build directory, and `main.cpp` loads them via the relative path `"../src/vShader.glsl"` — the executable is expected to be run with its working directory set to `build/`.

`Wireframe` (`include/wireframe.h`, `src/wireframe.cpp`) is a stubbed-out class with no members yet, intended for debug collider visualization.

### Third-party dependencies

`dependencies/` vendors GLFW 3.4, GLEW 2.1.0, and GLM source trees directly, plus Jolt Physics v5.6.0 as a **git submodule** (`dependencies/JoltPhysics`). Treat all of these as read-only.

Note: many comments and log messages in the codebase are written in Korean.
