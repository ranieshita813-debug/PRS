# Module dependencies (low to high)

```
L0  Core
L1  Platform, Jobs, VFS
L2  RHI, ECS
L3  Renderer, Assets
L4  Scene, Physics, Animation, Audio, AI
L5  World, Scripting, Network, UI
L6  Gameplay
L7  Engine
```
Rule: depend only downward. Renderer never includes Vulkan headers; it talks to RHI.
