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

# Generated hooks
set(VkHooks vkCmdDrawIndexed)

# White list
set(VkWhitelist vkAllocateCommandBuffers,vkFreeCommandBuffers,vkCmdExecuteCommands,vkBeginCommandBuffer,vkEndCommandBuffer)

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
        -hook ${VkHooks}
        -whitelist ${VkWhitelist}
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
        -o Layer/Source/CommandBuffer.Gen.cpp
        -hook ${VkHooks}
        -whitelist ${VkWhitelist}
)

# Create layer
add_library(
    Backends.Vulkan.Layer SHARED
    Layer/Source/Layer.cpp
    Layer/Source/DeviceDispatchTable.cpp
    Layer/Source/InstanceDispatchTable.cpp
    Layer/Source/Device.cpp
    Layer/Source/Instance.cpp
    Layer/Source/CommandBuffer.cpp

    # Generated
    Layer/Source/CommandBuffer.Gen.cpp

    # Generated dependency headers
    Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
)

# Link against backend
target_link_libraries(Backends.Vulkan.Layer Libraries.Backend)

# Include directories
target_include_directories(
    Backends.Vulkan.Layer PUBLIC
    Layer/Include ${CMAKE_CURRENT_BINARY_DIR}/Layer/Include
)

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Layer VulkanHeaders)

# Copy layer definition (configuration time)
configure_file(Layer/Resources/VK_GPUOpen_GBV.json ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/VK_GPUOpen_GBV.json COPYONLY)

#----- Test Layer -----#

# Create test layer
add_library(
    Backends.Vulkan.Tests.UserDataLayer SHARED
    Tests/Layer/UserDataLayer.cpp
)

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Tests.UserDataLayer VulkanHeaders)

# Copy layer definition (configuration time)
configure_file(Tests/Resources/VK_GPUOpen_Test_UserDataLayer.json ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/VK_GPUOpen_Test_UserDataLayer.json COPYONLY)

#----- Test Application -----#

# Create test app
add_executable(
    Backends.Vulkan.Tests
    Tests/Source/Main.cpp
    Tests/Source/Loader.cpp
    Tests/Source/UserData.cpp
    Tests/Source/Layer.cpp
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
add_dependencies(
    Backends.Vulkan.Tests
    Backends.Vulkan.Tests.UserDataLayer
    Backends.Vulkan.Layer
)
