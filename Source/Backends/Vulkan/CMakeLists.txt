#----- Generator -----#

# Create generator
add_executable(
    Backends.Vulkan.Generator
    Generator/Source/Generator.cpp
    Generator/Source/CommandBuffer.cpp
    Generator/Source/CommandBufferDispatchTable.cpp
    Generator/Source/DeepCopyObjects.cpp
    Generator/Source/DeepCopy.cpp
)

# Local links
target_link_libraries(Backends.Vulkan.Generator Libraries.Common)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(Backends.Vulkan.Generator PRIVATE /EHa)
endif()

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Generator ArgParse)
ExternalProject_Link(Backends.Vulkan.Generator SPIRVHeaders)
ExternalProject_Link(Backends.Vulkan.Generator VulkanHeaders)
ExternalProject_Link(Backends.Vulkan.Generator TinyXML2 tinyxml2)

#----- Layer -----#

set(VKXML                              ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanHeaders/registry/vk.xml)
set(CommandBufferTemplate              ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/CommandBuffer.cppt)
set(CommandBufferDispatchTableTemplate ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/CommandBufferDispatchTable.ht)
set(DeepCopyObjectsTemplate            ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopyObjects.ht)
set(DeepCopyTemplate                   ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopy.cppt)

# Generated hooks
set(VkHooks vkCmdDrawIndexed)

# White list
set(VkWhitelist vkAllocateCommandBuffers,vkFreeCommandBuffers,vkCmdExecuteCommands,vkBeginCommandBuffer,vkEndCommandBuffer,vkCmdBindPipeline)

# Object list
set(VkObjects VkGraphicsPipelineCreateInfo,VkComputePipelineCreateInfo,VkShaderModuleCreateInfo)

# Generate the deep copy objects
add_custom_command(
    OUTPUT Layer/Include/Backends/Vulkan/DeepCopyObjects.Gen.h
    DEPENDS Backends.Vulkan.Generator
    DEPENDS
        ${DeepCopyObjectsTemplate}
    COMMAND Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -template ${DeepCopyObjectsTemplate}
        -gentype deepcopyobjects
        -object ${VkObjects}
        -o Layer/Include/Backends/Vulkan/DeepCopyObjects.Gen.h
)

# Generate the deep copies
add_custom_command(
    OUTPUT Layer/Source/DeepCopy.Gen.cpp
    DEPENDS Backends.Vulkan.Generator
    DEPENDS
        ${DeepCopyTemplate}
    COMMAND Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -template ${DeepCopyTemplate}
        -gentype deepcopy
        -object ${VkObjects}
        -o Layer/Source/DeepCopy.Gen.cpp
)

# Generate the command buffer dispatch table
add_custom_command(
    OUTPUT Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
    DEPENDS Backends.Vulkan.Generator
    DEPENDS
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
    Layer/Source/ShaderModule.cpp
    Layer/Source/Pipeline.cpp
    Layer/Source/Queue.cpp
    Layer/Source/Compiler/SpvModule.cpp
    Layer/Source/Compiler/ShaderCompiler.cpp
    Layer/Source/Compiler/PipelineCompiler.cpp
    Layer/Source/Controllers/InstrumentationController.cpp

    # Generated
    Layer/Source/CommandBuffer.Gen.cpp
    Layer/Source/DeepCopy.Gen.cpp

    # Generated dependency headers
    Layer/Include/Backends/Vulkan/DeepCopyObjects.Gen.h
    Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
)

# Link against backend
target_link_libraries(Backends.Vulkan.Layer PUBLIC Libraries.Backend Libraries.Bridge Libraries.Common)

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

# Hlsl files
set(Generated "")
Project_AddHLSL(Generated cs_6_0 Tests/Data/WriteUAV.hlsl Tests/Include/Data/WriteUAV.h kSPIRVWriteUAV)

# Create test app
add_executable(
    Backends.Vulkan.Tests
    Tests/Source/Main.cpp
    Tests/Source/Loader.cpp
    Tests/Source/UserData.cpp
    Tests/Source/Layer.cpp
    Tests/Source/VMA.cpp

    # Generated
    ${Generated}
)

# Includes
target_include_directories(Backends.Vulkan.Tests PUBLIC Layer/Include Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Links
target_link_libraries(Backends.Vulkan.Tests Libraries.Common Backends.Vulkan.Layer)

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Tests Catch2)
ExternalProject_Link(Backends.Vulkan.Tests VMA)
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
)
