#----- Generator -----#

# Create generator
add_executable(
    Backends.Vulkan.Generator
    Generator/Source/Generator.cpp
    Generator/Source/CommandBuffer.cpp
    Generator/Source/CommandBufferDispatchTable.cpp
    Generator/Source/DeepCopyObjects.cpp
    Generator/Source/DeepCopy.cpp
    Generator/Source/Spv.cpp
)

# IDE source discovery
SetSourceDiscovery(Backends.Vulkan.Generator CXX Generator)

# Local links
target_link_libraries(Backends.Vulkan.Generator PUBLIC Libraries.Common)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(Backends.Vulkan.Generator PRIVATE /EHa)
endif()

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Generator ArgParse)
ExternalProject_Link(Backends.Vulkan.Generator SPIRVHeaders)
ExternalProject_Link(Backends.Vulkan.Generator VulkanHeaders)
ExternalProject_Link(Backends.Vulkan.Generator TinyXML2 tinyxml2)
ExternalProject_Link(Backends.Vulkan.Generator JSON)

#----- Layer -----#

set(VKXML                              ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanHeaders/registry/vk.xml)
set(CommandBufferTemplate              ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/CommandBuffer.cppt)
set(CommandBufferDispatchTableTemplate ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/CommandBufferDispatchTable.ht)
set(DeepCopyObjectsTemplate            ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopyObjects.ht)
set(DeepCopyTemplate                   ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopy.cppt)
set(SpvTemplate                        ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Spv.ht)

# Generated hooks
set(VkHooks vkCmdDrawIndexed)

# White list
set(VkWhitelist vkAllocateCommandBuffers,vkFreeCommandBuffers,vkCmdExecuteCommands,vkBeginCommandBuffer,vkResetCommandBuffer,vkEndCommandBuffer,vkCmdBindPipeline,vkCmdBindDescriptorSets)

# Object list
set(VkObjects VkGraphicsPipelineCreateInfo,VkComputePipelineCreateInfo,VkShaderModuleCreateInfo,VkApplicationInfo,VkDeviceCreateInfo)

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

# Generate the spv utilities
add_custom_command(
    OUTPUT Layer/Include/Backends/Vulkan/Compiler/Spv.Gen.h
    DEPENDS Backends.Vulkan.Generator
    DEPENDS
        ${SpvTemplate}
    COMMAND Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -spvjson ${CMAKE_SOURCE_DIR}/ThirdParty/SPIRVHeaders/include/spirv/unified1/spirv.core.grammar.json
        -template ${SpvTemplate}
        -gentype spv
        -o Layer/Include/Backends/Vulkan/Compiler/Spv.Gen.h
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
    Layer/Source/DescriptorSet.cpp
    Layer/Source/ShaderModule.cpp
    Layer/Source/QueueSegment.cpp
    Layer/Source/Pipeline.cpp
    Layer/Source/Fence.cpp
    Layer/Source/Queue.cpp
    Layer/Source/RenderPass.cpp
    Layer/Source/Allocation/DeviceAllocator.cpp
    Layer/Source/Compiler/SpvModule.cpp
    Layer/Source/Compiler/ShaderCompiler.cpp
    Layer/Source/Compiler/ShaderCompilerDebug.cpp
    Layer/Source/Compiler/PipelineCompiler.cpp
    Layer/Source/Controllers/InstrumentationController.cpp
    Layer/Source/Export/ShaderExportHost.cpp
    Layer/Source/Export/ShaderExportStreamAllocator.cpp
    Layer/Source/Export/ShaderExportDescriptorAllocator.cpp
    Layer/Source/Export/ShaderExportStreamer.cpp
    Layer/Source/Symbolizer/ShaderSGUIDHost.cpp
    Layer/Source/Compiler/SpvSourceMap.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockAnnotation.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockCapability.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockDebugStringSource.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockFunction.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockTypeConstantVariable.cpp
    Layer/Source/Compiler/Utils/SpvUtilShaderExport.cpp
    Layer/Source/Compiler/SpvPhysicalBlockScan.cpp
    Layer/Source/Compiler/SpvPhysicalBlockTable.cpp

    # VMA implementation
    Layer/Source/VMA.cpp

    # Generated
    Layer/Source/CommandBuffer.Gen.cpp
    Layer/Source/DeepCopy.Gen.cpp

    # Generated dependency headers
    Layer/Include/Backends/Vulkan/DeepCopyObjects.Gen.h
    Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
    Layer/Include/Backends/Vulkan/Compiler/Spv.Gen.h
)

# IDE source discovery
SetSourceDiscovery(Backends.Vulkan.Layer CXX Layer)

# Link against backend
target_link_libraries(
    Backends.Vulkan.Layer PUBLIC
    Libraries.Backend
    Libraries.Bridge
    Libraries.Common

    # Win32
    $<$<PLATFORM_ID:Windows>:Dbghelp>
)

# Include directories
target_include_directories(
    Backends.Vulkan.Layer PUBLIC
    Layer/Include ${CMAKE_CURRENT_BINARY_DIR}/Layer/Include
)

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Layer VulkanHeaders)
ExternalProject_Link(Backends.Vulkan.Layer VMA)

# Validation dependencies
ExternalProject_Link(Backends.Vulkan.Layer SPIRVTools SPIRV-Tools)

# Copy layer definition (configuration time)
ConfigureOutput(Layer/Resources/VK_LAYER_GPUOPEN_GBV.json VK_LAYER_GPUOPEN_GBV.json)

#----- Discovery -----#

# Create feature
add_library(
    Backends.Vulkan.Discovery SHARED
    Discovery/Source/Plugin.cpp
    Discovery/Source/VulkanDiscoveryListener.cpp
    Vulkan.xml
)

# IDE source discovery
SetSourceDiscovery(Backends.Vulkan.Discovery CXX Discovery)

# Includes
target_include_directories(Backends.Vulkan.Discovery PUBLIC Discovery/Include)

# Links
target_link_libraries(Backends.Vulkan.Discovery PUBLIC Libraries.Discovery Backends.Vulkan.Layer)

# Set all outputs to Plugins/.
PostBuildCopy(Backends.Vulkan.Discovery Plugins)

# Copy spec xml
ConfigureOutput(Vulkan.xml Plugins/Vulkan.xml)

#----- Test Device -----#

# Create test layer
add_library(
    Backends.Vulkan.TestDevice STATIC
    TestDevice/Source/Device.cpp
    TestDevice/Source/VMA.cpp
)

# IDE source discovery
SetSourceDiscovery(Backends.Vulkan.TestDevice CXX TestDevice)

# Include
target_include_directories(Backends.Vulkan.TestDevice PUBLIC TestDevice/Include)

# Link
target_link_libraries(Backends.Vulkan.TestDevice PUBLIC Backends.Vulkan.Layer Test.Device)

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.TestDevice VulkanHeaders)
ExternalProject_Link(Backends.Vulkan.TestDevice VMA)

#----- Test Layer -----#

# Create test layer
add_library(
    Backends.Vulkan.Tests.UserDataLayer SHARED
    Tests/Layer/UserDataLayer.cpp
)

# IDE source discovery
SetSourceDiscovery(Backends.Vulkan.Tests.UserDataLayer CXX Tests/Layer)

# Setup dependencies
ExternalProject_Link(Backends.Vulkan.Tests.UserDataLayer VulkanHeaders)

# Copy layer definition (configuration time)
ConfigureOutput(Tests/Resources/VK_GPUOpen_Test_UserDataLayer.json VK_GPUOpen_Test_UserDataLayer.json)

#----- Test Application -----#

# Hlsl files
set(Generated "")
Project_AddHLSL(Generated cs_6_0 "-Od" Tests/Data/WriteUAV.hlsl Tests/Include/Data/WriteUAV kSPIRVWriteUAV)
Project_AddHLSL(Generated cs_6_0 "-Od" Tests/Data/WriteUAVNegative.hlsl Tests/Include/Data/WriteUAVNegative kSPIRVWriteUAVNegative)

# Generate the schema
Project_AddShaderSchema(Generated Tests/Schemas/WritingNegativeValue.xml Tests/Include/Schemas)

# Create test app
add_executable(
    Backends.Vulkan.Tests
    Tests/Source/Main.cpp
    Tests/Source/Loader.cpp
    Tests/Source/UserData.cpp
    Tests/Source/Layer/Layer.cpp
    Tests/Source/Layer/OffsetStoresByOne.cpp
    Tests/Source/Layer/WritingNegativeValue.cpp
    Tests/Source/VMA.cpp

    # Generated
    ${Generated}
)

# IDE source discovery
SetSourceDiscovery(Backends.Vulkan.Tests CXX Tests)

# Includes
target_include_directories(Backends.Vulkan.Tests PUBLIC Layer/Include Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Links
target_link_libraries(Backends.Vulkan.Tests PUBLIC Libraries.Common Backends.Vulkan.Layer)

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
