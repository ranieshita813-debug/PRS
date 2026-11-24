if(ANDROID)
    # Android: ANativeWindow + native_app_glue provide the window, touch events
    # and the android_main() entry point.
    prs_add_module(Platform DEPS_PUBLIC PRS::Core)

    set(PRS_APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
    add_library(prs_native_app_glue STATIC ${PRS_APP_GLUE_DIR}/android_native_app_glue.c)
    target_include_directories(prs_native_app_glue PUBLIC ${PRS_APP_GLUE_DIR})
    target_link_libraries(PRS_Platform PUBLIC prs_native_app_glue android log)
else()
    prs_add_module(Platform DEPS_PUBLIC PRS::Core DEPS_PRIVATE glfw)
endif()
