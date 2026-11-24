# PRS Roadmap

1. **Phase 1**: Core, Platform, Vulkan RHI (`RHI/Private/Vulkan`), basic Renderer: lit 3D scene.
   - Core done (math, containers, delegates, allocators, log, clock).
   - RHI done: Vulkan instance/device selection, swapchain, buffers, textures,
     graphics pipelines, command lists, fences/semaphores.
   - Renderer done: forward pass with camera, one directional light, depth
     buffer and present. PBR, shadows, render graph and offscreen targets remain.
2. **Phase 2**: ECS, Assets + simple cooker, Scene, Editor basics (ImGui).
   - ECS done: entity generations, dense component pools, views, deferred commands, staged scheduler.
   - Scene done: hierarchy, camera/light/renderable components, text serializer, prefabs.
   - Assets/cooker and the editor remain.
3. **Phase 3**: Physics (Jolt), Animation, Audio, Scripting, small playable game.
   - Physics done as a self-contained solver (gravity, sphere/box contacts, friction,
     sleeping, raycast, fixed step). Jolt integration still planned for large scenes.
   - Animation done: skeleton, keyframe clips, pose blending, crossfade graph,
     skinning palette, name-based retargeting.
   - Audio and Scripting remain.
4. **Phase 4**: RenderGraph, job-driven ECS scheduling, GPU-driven culling.
5. **Phase 5**: World streaming, AI/traffic/crowd, vehicles, HLOD, floating origin.

Replace each remaining stub module's `ModuleName()` placeholder with real code as you reach its phase.
