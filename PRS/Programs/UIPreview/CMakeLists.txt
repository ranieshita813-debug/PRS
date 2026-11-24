set(PRS_MOBILE_SHELL_UI_DIR ${CMAKE_SOURCE_DIR}/Games/MobileShell/Source)

add_executable(UIPreview main.cpp ${PRS_MOBILE_SHELL_UI_DIR}/MobileShellUI.cpp)
target_include_directories(UIPreview PRIVATE ${PRS_MOBILE_SHELL_UI_DIR})
target_link_libraries(UIPreview PRIVATE PRS::UI)
