add_library(PRS_Developer INTERFACE)
target_link_libraries(PRS_Developer INTERFACE PRS::Core)
# Compile-out in shipping builds: add PRS_SHIPPING definition and skip linking this target.
