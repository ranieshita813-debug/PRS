add_executable(PRS_UnitTests
    Unit/main.cpp
    Unit/CoreTests.cpp
    Unit/MathTests.cpp
    Unit/ECSTests.cpp
    Unit/SceneTests.cpp
    Unit/PhysicsTests.cpp
    Unit/AnimationTests.cpp
    Unit/JobTests.cpp
    Unit/UITests.cpp)
target_link_libraries(PRS_UnitTests PRIVATE
    PRS::Core
    PRS::Jobs
    PRS::ECS
    PRS::Scene
    PRS::Physics
    PRS::Animation
    PRS::UI)
add_test(NAME UnitTests COMMAND PRS_UnitTests)
