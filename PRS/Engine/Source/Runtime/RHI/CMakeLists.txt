prs_add_module(RHI DEPS_PUBLIC PRS::Core PRS::Platform)

# The Vulkan backend lives behind the RHI interfaces. When the Vulkan loader and
# headers are not present the module still builds; CreateDevice() then logs that
# rendering is unavailable and returns null.
if(Vulkan_FOUND)
    target_compile_definitions(PRS_RHI PUBLIC PRS_HAS_VULKAN=1)
    target_link_libraries(PRS_RHI PUBLIC Vulkan::Vulkan)
else()
    message(STATUS "PRS: Vulkan not found - RHI will build without a graphics backend")
endif()
