# PRS Engine

A modular, layered C++20 game engine aimed at open-world games (GTA-style).
Built Vulkan-first behind a Render Hardware Interface (RHI).

## Build

Requirements: CMake 3.20+, a C++20 compiler, Git, Ninja (recommended).
Linux also needs X11/Wayland dev packages (see `.github/workflows/ci.yml`).
For rendering you need a Vulkan 1.1 driver + loader and the Vulkan SDK headers
(`find_package(Vulkan)`); without them the engine builds and runs headless.

```
cmake --preset debug
cmake --build --preset debug
./Intermediate/debug/bin/MyOpenWorld        # Windows: MyOpenWorld.exe
ctest --test-dir Intermediate/debug
```

Controls in the sample game: **ESC** quits, **WASD** moves the camera.

The sample renders a lit scene through the Vulkan forward renderer: a textured
ground plane, a spinning cube and a textured sphere loaded from
`Games/MyOpenWorld/Assets/Models/sphere.obj` (OBJ + MTL + TGA). Shaders are
compiled from `Engine/Shaders/*.glsl` by
`glslangValidator` when the Vulkan SDK is installed, otherwise the prebuilt
SPIR-V in `Engine/Shaders/Compiled` is copied next to the binaries.

## Status

| Module | State |
|---|---|
| Core (types, assert, log, allocators, clock, math, containers, delegates) | implemented |
| Platform (window, input) | implemented (GLFW on desktop, ANativeWindow + touch on Android) |
| Jobs (thread pool) | implemented (basic) |
| ECS (entities + generations, dense component pools, views, commands, system scheduler) | implemented |
| Scene (hierarchy, camera/light/renderable components, text serializer, prefabs) | implemented |
| Physics (fixed step, gravity, sphere/box contacts, friction, sleeping, raycast) | implemented (no Jolt dependency yet) |
| Animation (skeleton, keyframe clips, pose blending, crossfade graph, skinning palette, name retargeting) | implemented |
| Engine (Application, fixed-timestep loop) | implemented |
| UI (touch widgets, layout, scrolling, draw list, software rasterizer) | implemented |
| RHI (Vulkan 1.1 backend: instance/device, swapchain, buffers, textures, pipelines, command lists, fences) | implemented |
| Renderer (forward pass: camera, directional light, textured materials, mesh draws, depth buffer, present) | implemented, see [Docs/Rendering.md](Docs/Rendering.md) |
| Assets (OBJ/MTL meshes, TGA/BMP textures, search-path cache) | implemented (textures/meshes), cooker and glTF still stubs |
| Audio / Scripting / Network / World / AI | compiling stubs, see `Docs/Roadmap.md` |

## Runtime quick start

```cpp
#include "PRS/Physics/PhysicsWorld.h"
#include "PRS/Scene/Scene.h"
#include "PRS/Scene/SceneSerializer.h"

Scene::Scene scene("Level");
ECS::Entity ground = scene.CreateEntity("Ground");
scene.World().Emplace<Physics::BoxCollider>(ground, Physics::BoxCollider{{20, 0.5f, 20}});

ECS::Entity ball = scene.CreateEntity("Ball");
scene.World().Emplace<Physics::SphereCollider>(ball, Physics::SphereCollider{0.5f});
Physics::RigidBody body;
body.SetMass(1.0f);
scene.World().Emplace<Physics::RigidBody>(ball, body);

Physics::PhysicsWorld physics;
physics.Step(scene.World(), 1.0f / 60.0f);
Scene::SceneSerializer::Save(scene, "Saved/level.prsscene");
```

Components are added with `Emplace`, read with `Get`/`TryGet`, and iterated with
`Query<A, B>().ForEach([](ECS::Entity e, A& a, B& b) { ... })`. Structural changes
made inside a system go through `ECS::Commands` and are flushed by the scheduler.

## Android interface

`Games/MobileShell` is a touch interface (Home / Settings / About) built on the
`PRS::UI` module, with an Android platform backend in
`Engine/Source/Runtime/Platform/Private/Android` and a Gradle project in
`Platforms/Android`. See [Docs/Android.md](Docs/Android.md).

The mobile shell does not create a Vulkan device yet, so its interface is
rasterised on the CPU and presented through `ANativeWindow_lock()` on device. To see it on a
desktop without a phone:

```
./Intermediate/debug/bin/UIPreview Saved    # writes Saved/ui-*.png
```

## Layering (enforced by CMake `DEPS_*`)

See `Docs/Architecture/Module-Dependencies.md`. A module may only include modules below it.
