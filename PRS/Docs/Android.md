# Android interface

The mobile interface is built on four pieces, each replacing a stub or filling a
gap in the engine:

| Layer | Location | What it does |
|---|---|---|
| UI toolkit | `Engine/Source/Runtime/UI` | Touch-first immediate-mode widgets, layout, scrolling, draw list |
| Android platform | `Engine/Source/Runtime/Platform/Private/Android` | `android_main`, ANativeWindow, touch events, lifecycle |
| Mobile shell | `Games/MobileShell` | The interface itself (Home / Settings / About) plus presentation |
| Preview tool | `Programs/UIPreview` | Renders the interface to PNG on a desktop, no device needed |

## Why a software presenter

The RHI and Renderer modules are still Phase 1 stubs (`VulkanDevice.cpp` and
friends contain only TODOs), so there is no GPU path to draw with yet. The UI
therefore records a backend-agnostic `DrawList` and a small CPU rasterizer turns
it into pixels, which are copied into the Android surface with
`ANativeWindow_lock()` / `ANativeWindow_unlockAndPost()`.

That path is fine for menus: it keeps the interface usable on a real device now,
and it is a reference implementation the Vulkan UI pass can be checked against.
When the RHI lands, `SoftwareCanvas` is swapped for a Vulkan pass that consumes
the same `DrawList` (vertices, indices, commands, clip rects).

## Two things to know before you poke at it

1. Nothing is displayed on desktop windows yet — the engine has no renderer.
   Use `UIPreview` to see the interface (it produces real PNGs).
2. Text is a built-in 5x7 bitmap font (uppercase, digits, punctuation). Good for
   UI labels and deliberately tiny; swap in a TTF/SDF atlas later.

## Build for a device

Open `Platforms/Android` in Android Studio (Gradle 8.5+, NDK 27, CMake 3.22), or
build from a shell where Gradle is installed:

```
cd Platforms/Android
gradle :app:installDebug
```

Gradle drives the engine's own `CMakeLists.txt`; `Games/MobileShell` builds as
`libmobile_shell.so` and `android.app.NativeActivity` loads it. No Java code is
needed: `AndroidManifest.xml` sets `android.app.lib_name` and
`hasCode="false"`.

A Gradle wrapper is not checked in, so a first build needs either Android
Studio's sync or a locally installed Gradle.

## Preview on desktop

```
cmake --preset debug
cmake --build --preset debug --parallel
./Intermediate/debug/bin/UIPreview Saved      # writes Saved/ui-*.png
```

## Writing a screen

`Games/MobileShell/Source/MobileShellUI.cpp` is the whole interface. A screen is
just a function that calls widgets:

```cpp
ui.Heading("GRAPHICS");
ui.Toggle("shadows", "SHADOWS", state.shadows);
ui.Slider("scale", "RENDER SCALE", state.renderScale, 0.5f, 2.0f);
if (ui.Button("reset", "RESET DEFAULTS")) state.Notify("DEFAULTS RESTORED");
```

Widget ids are strings hashed internally, so they must stay stable between
frames. `bool` returns mean "activated this frame" (tapped, toggled, changed).

Frame order matters: age the input, feed the touch events, then build the UI.

```cpp
m_input.BeginFrame(dt);           // clears press/release edges
PumpPointer();                    // Platform::Touch::Poll -> m_input
m_ui.BeginFrame(size, m_input, dt);
MobileShell::Draw(m_ui, surface, state);
m_ui.EndFrame();
```

## Layout rules the theme enforces

- All metrics are in dp (`Theme::Dp`), scaled by device density, so the same
  layout works on a 1080p phone and a tablet.
- Interactive rows are at least `minTouchTarget` = 48dp tall (Android's
  accessibility minimum).
- Drags cancel taps after 12dp of travel, and a drag inside a scrolling view
  scrolls instead of pressing widgets.
- Scrolling is clamped to the content height in both `BeginView` and `EndView`.

## Limits / next steps

- CPU rasterisation only; expect menu-fast, not 3D-fast. The Vulkan UI pass is
  the next step once `RHI/Device` exists.
- No text wrapping, no localisation, no accessibility (TalkBack) layer, no TTF.
- Multitouch is tracked (up to 64 events per frame) but widgets only act on the
  primary pointer.
- The Android build has not been compiled in this environment (no NDK here), so
  treat `Platforms/Android` as the scaffold to validate on your machine.
