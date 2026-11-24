# The UI layer records a backend-agnostic DrawList (plus a reference software
# rasterizer), so it depends on Core only. The Vulkan UI pass will live in
# Renderer and consume the same DrawList.
prs_add_module(UI DEPS_PUBLIC PRS::Core)
