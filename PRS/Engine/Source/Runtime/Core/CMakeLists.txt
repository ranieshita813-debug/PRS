prs_add_module(Core)
find_package(Threads REQUIRED)
target_link_libraries(PRS_Core PUBLIC Threads::Threads)
