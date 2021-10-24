#----- Generator -----#

# Create generator
add_executable(
    Backends.Vulkan.Generator
    Generator/Source/Generator.cpp
    Generator/Source/CommandBuffer.cpp
    Generator/Source/CommandBufferDispatchTable.cpp
)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(Backends.Vulkan.Generator PRIVATE /EHa)
endif()

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Generator ArgParse)
ExternalProject_Link(Backends.Vulkan.Generator TinyXML2 tinyxml2)

#----- Layer -----#

set(VKXML                              ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanHeaders/registry/vk.xml)
set(CommandBufferTemplate              ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/CommandBuffer.cppt)
set(CommandBufferDispatchTableTemplate ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/CommandBufferDispatchTable.ht)

# Generate the command buffer dispatch table
add_custom_command(
    OUTPUT Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
    DEPENDS Backends.Vulkan.Generator
    DEPENDS
        ${VKXML}
        ${CommandBufferDispatchTableTemplate}
    COMMAND Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -template ${CommandBufferDispatchTableTemplate}
        -gentype commandbufferdispatchtable
        -o Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
)

# Generate the command buffer hooks
add_custom_command(
    OUTPUT Layer/Source/CommandBuffer.Gen.cpp
    DEPENDS Backends.Vulkan.Generator
    DEPENDS
        ${VKXML}
        ${CommandBufferTemplate}
    COMMAND Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -template ${CommandBufferTemplate}
        -gentype commandbuffer
        -whitelist vkAllocateCommandBuffers,vkFreeCommandBuffers,vkCmdExecuteCommands
        -o Layer/Source/CommandBuffer.Gen.cpp
)

# Create test layer
add_library(
    Backends.Vulkan.Layer SHARED
    Layer/Source/Layer.cpp
    Layer/Source/CommandBuffer.Gen.cpp

    # Dependency headers
    Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
)

# Include directories
target_include_directories(Backends.Vulkan.Layer PUBLIC Layer/Include ${CMAKE_CURRENT_BINARY_DIR}/Layer/Include)

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Layer VulkanHeaders)

# Copy layer definition (configuration time)
configure_file(Layer/Resources/VK_GPUOpen_GBV.json ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/VK_GPUOpen_GBV.json COPYONLY)

#----- Test Layer -----#

# Create test layer
add_library(
    Backends.Vulkan.Tests.UserDataLayer SHARED
    Tests/UserDataLayer.cpp
)

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Tests.UserDataLayer VulkanHeaders)

# Copy layer definition (configuration time)
configure_file(Tests/user_data_layer.json ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/user_data_layer.json COPYONLY)

#----- Test Application -----#

# Create test app
add_executable(
    Backends.Vulkan.Tests
    Tests/Loader.cpp
    Tests/UserData.cpp
)

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Tests Catch2)
ExternalProject_Link(Backends.Vulkan.Tests VulkanLoader vulkan-1)

# Compiler definitions
target_compile_definitions(
    Backends.Vulkan.Tests PRIVATE
    CATCH_CONFIG_ENABLE_BENCHMARKING # Enable benchmarking
)

# Add dependency on the layer
add_dependencies(Backends.Vulkan.Tests Backends.Vulkan.Tests.UserDataLayer)
