
# Vulkan headers
ExternalProject_Add(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers
    GIT_TAG v1.2.189
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/VulkanHeaders
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCMAKE_INSTALL_LIBDIR=${CMAKE_BINARY_DIR}/External/lib
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -G ${CMAKE_GENERATOR}
)

# MASM support
if (MINGW)
    set(UseMASM 0)
else()
    set(UseMASM 1)
endif()

# Vulkan loader
ExternalProject_Add(
    VulkanLoader
    DEPENDS VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader
    GIT_TAG sdk-1.2.189.0
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/VulkanLoader
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCMAKE_INSTALL_LIBDIR=${CMAKE_BINARY_DIR}/External/lib
        -DVULKAN_HEADERS_INSTALL_DIR=${CMAKE_BINARY_DIR}/External
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        -DUSE_MASM=${UseMASM}
        -DCMAKE_ASM_MASM_COMPILER_WORKS=${UseMASM}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -DBUILD_TESTS=OFF
        -G ${CMAKE_GENERATOR}
)

# MinGW workaround
if (MINGW)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/VulkanLoader/loader/winres.h "#include <Windows.h>")
endif()
