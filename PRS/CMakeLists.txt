cmake_minimum_required(VERSION 3.20)
project(PRS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Android builds only the engine + the mobile shell, so tools and tests default off.
if(ANDROID)
    option(PRS_BUILD_TESTS "Build unit tests" OFF)
    option(PRS_BUILD_PROGRAMS "Build tools (cooker, shader compiler...)" OFF)
else()
    option(PRS_BUILD_TESTS "Build unit tests" ON)
    option(PRS_BUILD_PROGRAMS "Build tools (cooker, shader compiler...)" ON)
endif()

include(CMake/PRSModule.cmake)
include(CMake/Dependencies.cmake)

add_subdirectory(Engine/Source/Runtime)
add_subdirectory(Engine/Source/Developer)
add_subdirectory(Engine/Source/Editor)
if(PRS_BUILD_PROGRAMS)
    add_subdirectory(Programs)
endif()
add_subdirectory(Games/MobileShell)
if(NOT ANDROID)
    add_subdirectory(Games/MyOpenWorld)
endif()
if(PRS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

# --- Shaders ----------------------------------------------------------------
# Prebuilt SPIR-V lives in Engine/Shaders/Compiled and is copied next to the
# binaries. When the Vulkan SDK's glslangValidator is available the shaders are
# rebuilt from the .glsl sources on every build instead.
set(PRS_SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Shaders)
file(GLOB PRS_PREBUILT_SHADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Shaders/Compiled/*.spv)
if(PRS_PREBUILT_SHADERS)
    file(COPY ${PRS_PREBUILT_SHADERS} DESTINATION ${PRS_SHADER_OUTPUT_DIR})
endif()

if(GLSLANG_VALIDATOR)
    add_custom_target(PRS_Shaders
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PRS_SHADER_OUTPUT_DIR}
        COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.1 -o ${PRS_SHADER_OUTPUT_DIR}/forward.vert.spv ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Shaders/Forward.vert.glsl
        COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.1 -o ${PRS_SHADER_OUTPUT_DIR}/forward.frag.spv ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Shaders/Forward.frag.glsl
        COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.1 -o ${PRS_SHADER_OUTPUT_DIR}/shadow.vert.spv ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Shaders/Shadow.vert.glsl
        COMMENT "Compiling PRS forward shaders"
        VERBATIM)
    if(TARGET MyOpenWorld)
        add_dependencies(MyOpenWorld PRS_Shaders)
    endif()
endif()
