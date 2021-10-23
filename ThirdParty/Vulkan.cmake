
# Vulkan headers
ExternalProject_Add(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers
    GIT_TAG sdk-1.2.189
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/VulkanHeaders
    USES_TERMINAL_INSTALL 0
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCMAKE_INSTALL_LIBDIR=${CMAKE_BINARY_DIR}/External/lib
)

# Vulkan loader
ExternalProject_Add(
    VulkanLoader
    DEPENDS VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader
    GIT_TAG sdk-1.2.189.0
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/VulkanLoader
    USES_TERMINAL_INSTALL 0
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCMAKE_INSTALL_LIBDIR=${CMAKE_BINARY_DIR}/External/lib
        -DVULKAN_HEADERS_INSTALL_DIR=${CMAKE_BINARY_DIR}/External
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
