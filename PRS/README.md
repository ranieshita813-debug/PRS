# PRS Engine

A modular, layered C++20 game engine aimed at open-world games (GTA-style).
Built Vulkan-first behind a Render Hardware Interface (RHI).

## Build

Requirements: CMake 3.20+, a C++20 compiler, Git, Ninja (recommended).
Linux also needs X11/Wayland dev packages (see `.github/workflows/ci.yml`).

```
cmake --preset debug
cmake --build --preset debug
./Intermediate/debug/bin/MyOpenWorld        # Windows: MyOpenWorld.exe
ctest --test-dir Intermediate/debug
```

Controls in the sample game: **ESC** quits, **WASD** logs movement.

## Status

| Module | State |
|---|---|
| Core (types, assert, log, allocators, clock) | implemented |
| Platform (window, input) | implemented (GLFW on desktop, ANativeWindow + touch on Android) |
| Jobs (thread pool) | implemented (basic) |
| Engine (Application, fixed-timestep loop) | implemented |
| UI (touch widgets, layout, scrolling, draw list, software rasterizer) | implemented |
| RHI / Renderer / ECS / Scene / Assets / Physics / ... | compiling stubs, see `Docs/Roadmap.md` |

## Android interface

`Games/MobileShell` is a touch interface (Home / Settings / About) built on the
`PRS::UI` module, with an Android platform backend in
`Engine/Source/Runtime/Platform/Private/Android` and a Gradle project in
`Platforms/Android`. See [Docs/Android.md](Docs/Android.md).

Because the Vulkan renderer is still a stub, the interface is rasterised on the
CPU and presented through `ANativeWindow_lock()` on device. To see it on a
desktop without a phone:

```
./Intermediate/debug/bin/UIPreview Saved    # writes Saved/ui-*.png
```

## Layering (enforced by CMake `DEPS_*`)

See `Docs/Architecture/Module-Dependencies.md`. A module may only include modules below it.
