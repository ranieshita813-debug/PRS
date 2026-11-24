set(PRS_MOBILE_SHELL_SOURCES
    Source/main.cpp
    Source/MobileShellUI.cpp
    Source/Presenter.cpp)

# On Android the same sources build into libmobile_shell.so, which
# android.app.NativeActivity loads (see Platforms/Android).
if(ANDROID)
    add_library(MobileShell SHARED ${PRS_MOBILE_SHELL_SOURCES})
    set_target_properties(MobileShell PROPERTIES OUTPUT_NAME "mobile_shell")
    # Keep the glue entry point alive when the linker garbage-collects symbols.
    target_link_options(MobileShell PRIVATE "-u" "ANativeActivity_onCreate")
    target_link_libraries(MobileShell PRIVATE PRS::Engine android log)
else()
    add_executable(MobileShell ${PRS_MOBILE_SHELL_SOURCES})
    target_link_libraries(MobileShell PRIVATE PRS::Engine)
endif()

target_include_directories(MobileShell PRIVATE Source)
