include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

if(NOT ANDROID)
    # --- GLFW (desktop window + input). Vulkan-ready: windows use GLFW_NO_API ---
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
    FetchContent_MakeAvailable(glfw)
endif()

# Android uses ANativeWindow + android_native_app_glue (see Engine/Source/Runtime/Platform).

# Later phases (uncomment as you reach them):
# FetchContent_Declare(JoltPhysics GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git GIT_TAG v5.2.0 SOURCE_SUBDIR Build)
# FetchContent_Declare(volk GIT_REPOSITORY https://github.com/zeux/volk.git GIT_TAG 1.3.295)
# FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.5)
# FetchContent_Declare(miniaudio GIT_REPOSITORY https://github.com/mackron/miniaudio.git GIT_TAG 0.11.21)
# FetchContent_Declare(meshoptimizer GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git GIT_TAG v0.22)
# FetchContent_Declare(recastnavigation GIT_REPOSITORY https://github.com/recastnavigation/recastnavigation.git GIT_TAG v1.6.0)
