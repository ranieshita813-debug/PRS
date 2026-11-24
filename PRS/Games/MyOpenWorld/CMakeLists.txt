add_executable(MyOpenWorld Source/main.cpp)
target_link_libraries(MyOpenWorld PRIVATE PRS::Engine)
# Make Content/ and Config/ available next to the binary
add_custom_command(TARGET MyOpenWorld POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Config $<TARGET_FILE_DIR:MyOpenWorld>/Config)
# Models and textures, next to the binary so the renderer's default "Assets"
# search path resolves them.
add_custom_command(TARGET MyOpenWorld POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Assets $<TARGET_FILE_DIR:MyOpenWorld>/Assets)
