
add_library(
    Backends.Vulkan.Tests.UserDataLayer SHARED
    Tests/UserDataLayer.cpp
)

configure_file(Tests/user_data_layer.json ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/user_data_layer.json COPYONLY)

ExternalProject_Link(Backends.Vulkan.Tests.UserDataLayer VulkanHeaders)

add_executable(
    Backends.Vulkan.Tests
    Tests/Loader.cpp
    Tests/UserData.cpp
)

add_dependencies(Backends.Vulkan.Tests Backends.Vulkan.Tests.UserDataLayer)

target_compile_definitions(
    Backends.Vulkan.Tests PRIVATE
    CATCH_CONFIG_ENABLE_BENCHMARKING # Enable benchmarking
)

ExternalProject_Link(Backends.Vulkan.Tests Catch2)
ExternalProject_Link(Backends.Vulkan.Tests VulkanLoader vulkan-1)
