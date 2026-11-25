add_executable(PRS_UnitTests
    Unit/main.cpp
    Unit/CoreTests.cpp
    Unit/MathTests.cpp
    Unit/ECSTests.cpp
    Unit/SceneTests.cpp
    Unit/PhysicsTests.cpp
    Unit/AnimationTests.cpp
    Unit/AssetTests.cpp
    Unit/ShadowTests.cpp
    Unit/JobTests.cpp
    Unit/UITests.cpp)
target_link_libraries(PRS_UnitTests PRIVATE
    PRS::Core
    PRS::Jobs
    PRS::ECS
    PRS::Scene
    PRS::Physics
    PRS::Animation
    PRS::Assets
    PRS::UI)
# Shadow.h is a header-only piece of the renderer (pure matrix math), so the
# tests include its public directory without pulling in the RHI.
target_include_directories(PRS_UnitTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/Source/Runtime/Renderer/Public)
add_test(NAME UnitTests COMMAND PRS_UnitTests)
