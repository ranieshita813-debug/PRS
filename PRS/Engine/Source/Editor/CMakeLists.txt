add_library(PRS_Editor INTERFACE)
target_link_libraries(PRS_Editor INTERFACE PRS::Engine)
# Phase 2: turn into a real library with ImGui, panels, undo/redo.
