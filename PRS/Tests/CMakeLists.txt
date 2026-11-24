add_executable(PRS_UnitTests
    Unit/main.cpp
    Unit/CoreTests.cpp
    Unit/JobTests.cpp
    Unit/UITests.cpp)
target_link_libraries(PRS_UnitTests PRIVATE PRS::Core PRS::Jobs PRS::UI)
add_test(NAME UnitTests COMMAND PRS_UnitTests)
