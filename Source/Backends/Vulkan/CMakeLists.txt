# 
# The MIT License (MIT)
# 
# Copyright (c) 2024 Advanced Micro Devices, Inc.,
# Fatalist Development AB (Avalanche Studio Group),
# and Miguel Petersen.
# 
# All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy 
# of this software and associated documentation files (the "Software"), to deal 
# in the Software without restriction, including without limitation the rights 
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
# of the Software, and to permit persons to whom the Software is furnished to do so, 
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all 
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 

#----- Generator -----#

# Create generator
add_executable(
    GRS.Backends.Vulkan.Generator
    Generator/Source/Generator.cpp
    Generator/Source/CommandBuffer.cpp
    Generator/Source/CommandBufferDispatchTable.cpp
    Generator/Source/DeepCopyObjects.cpp
    Generator/Source/DeepCopy.cpp
    Generator/Source/Spv.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.Vulkan.Generator CXX Generator)

# Local links
target_link_libraries(GRS.Backends.Vulkan.Generator PUBLIC GRS.Libraries.Common)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Backends.Vulkan.Generator PRIVATE /EHs)
endif()

# Setup dependencies
ExternalProject_Link(GRS.Backends.Vulkan.Generator ArgParse)
ExternalProject_Link(GRS.Backends.Vulkan.Generator SPIRVHeaders)
ExternalProject_Link(GRS.Backends.Vulkan.Generator VulkanHeaders)
ExternalProject_Link(GRS.Backends.Vulkan.Generator TinyXML2 tinyxml2)
ExternalProject_Link(GRS.Backends.Vulkan.Generator JSON)

#----- Layer -----#

set(VKXML                              ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanHeaders/registry/vk.xml)
set(CommandBufferTemplate              ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/CommandBuffer.cppt)
set(CommandBufferDispatchTableTemplate ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/CommandBufferDispatchTable.ht)
set(DeepCopyObjectsTemplate            ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopyObjects.ht)
set(DeepCopyTemplate                   ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopy.cppt)
set(SpvTemplate                        ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Spv.ht)

# TODO: Move all configuration to a json file, the same as the DX12 generator

# Generated hooks
set(
    VkHooks
    vkCmdDispatch
    vkCmdDraw
    vkCmdDrawIndexed
    vkCmdCopyBuffer
    vkCmdCopyImage
    vkCmdBlitImage
    vkCmdCopyBufferToImage
    vkCmdCopyBufferToImage2
    vkCmdCopyImageToBuffer
    vkCmdCopyImageToBuffer2
    vkCmdUpdateBuffer
    vkCmdFillBuffer
    vkCmdClearColorImage
    vkCmdClearDepthStencilImage
    vkCmdClearAttachments
    vkCmdResolveImage
    vkCmdBeginRenderPass
    vkCmdEndRenderPass
    vkCmdBeginRendering
    vkCmdBeginRenderingKHR
    vkCmdEndRendering
    vkCmdEndRenderingKHR
)

# White list
set(
    VkWhitelist
    vkAllocateCommandBuffers
    vkFreeCommandBuffers
    vkCmdExecuteCommands
    vkBeginCommandBuffer
    vkResetCommandBuffer
    vkResetCommandPool
    vkEndCommandBuffer
    vkCmdBindPipeline
    vkCmdBindDescriptorSets
    vkCmdDispatch
    vkCmdDispatchBase
    vkCmdDispatchIndirect
    vkCmdDraw
    vkCmdDrawIndexed
    vkCmdDrawIndexedIndirect
    vkCmdDrawIndexedIndirectCount
    vkCmdDrawIndirect
    vkCmdDrawIndirectCount
    vkCmdPushConstants
    vkCmdBeginRenderPass
    vkCmdEndRenderPass
    vkCmdPushDescriptorSetKHR
    vkCmdPushDescriptorSetWithTemplateKHR
    vkCmdWaitEvents
    vkCmdPipelineBarrier
    vkCmdSetEvent2
    vkCmdWaitEvents2
    vkCmdPipelineBarrier2
)

# Join lists
list(JOIN VkHooks "," VkHooksJoin)
list(JOIN VkWhitelist "," VkWhitelistJoin)

# Object list
set(VkObjects VkGraphicsPipelineCreateInfo,VkComputePipelineCreateInfo,VkShaderModuleCreateInfo,VkApplicationInfo,VkInstanceCreateInfo,VkDeviceCreateInfo,VkPipelineShaderStageCreateInfo,VkRenderPassCreateInfo2,VkRenderPassBeginInfo,VkDescriptorUpdateTemplateCreateInfo,VkDescriptorSetLayoutCreateInfo)

# Generate the deep copy objects
add_custom_command(
    OUTPUT Layer/Include/Backends/Vulkan/DeepCopyObjects.Gen.h
    DEPENDS GRS.Backends.Vulkan.Generator
    DEPENDS
        ${DeepCopyObjectsTemplate}
    COMMAND GRS.Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -template ${DeepCopyObjectsTemplate}
        -gentype deepcopyobjects
        -object ${VkObjects}
        -o Layer/Include/Backends/Vulkan/DeepCopyObjects.Gen.h
)

# Generate the deep copies
add_custom_command(
    OUTPUT Layer/Source/DeepCopy.Gen.cpp
    DEPENDS GRS.Backends.Vulkan.Generator
    DEPENDS
        ${DeepCopyTemplate}
    COMMAND GRS.Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -template ${DeepCopyTemplate}
        -gentype deepcopy
        -object ${VkObjects}
        -o Layer/Source/DeepCopy.Gen.cpp
)

# Generate the command buffer dispatch table
add_custom_command(
    OUTPUT Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
    DEPENDS GRS.Backends.Vulkan.Generator
    DEPENDS
        ${CommandBufferDispatchTableTemplate}
    COMMAND GRS.Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -template ${CommandBufferDispatchTableTemplate}
        -gentype commandbufferdispatchtable
        -o Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
        -hook ${VkHooksJoin}
        -whitelist ${VkWhitelistJoin}
)

# Generate the command buffer hooks
add_custom_command(
    OUTPUT Layer/Source/CommandBuffer.Gen.cpp
    DEPENDS GRS.Backends.Vulkan.Generator
    DEPENDS
        ${CommandBufferTemplate}
    COMMAND GRS.Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -template ${CommandBufferTemplate}
        -gentype commandbuffer
        -o Layer/Source/CommandBuffer.Gen.cpp
        -hook ${VkHooksJoin}
        -whitelist ${VkWhitelistJoin}
)

# Generate the spv utilities
add_custom_command(
    OUTPUT Layer/Include/Backends/Vulkan/Compiler/Spv.Gen.h
    DEPENDS GRS.Backends.Vulkan.Generator
    DEPENDS
        ${SpvTemplate}
    COMMAND GRS.Backends.Vulkan.Generator
        -vkxml ${VKXML}
        -spvjson ${CMAKE_SOURCE_DIR}/ThirdParty/SPIRVHeaders/include/spirv/unified1/spirv.core.grammar.json
        -template ${SpvTemplate}
        -gentype spv
        -o Layer/Include/Backends/Vulkan/Compiler/Spv.Gen.h
)

# Inbuilt modules
Project_AddHLSL(GeneratedInbuilt cs_6_0 "-Od" Layer/Modules/InbuiltTemplateModule.hlsl Layer/Include/Backends/Vulkan/Modules/InbuiltTemplateModule kSPIRVInbuiltTemplateModule)

# Create layer
add_library(
    GRS.Backends.Vulkan.Layer SHARED
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
    Layer/Source/FeatureProxies.cpp
    Layer/Source/Swapchain.cpp
    Layer/Source/Allocation/DeviceAllocator.cpp
    Layer/Source/Compiler/SpvModule.cpp
    Layer/Source/Compiler/ShaderCompiler.cpp
    Layer/Source/Compiler/ShaderCompilerDebug.cpp
    Layer/Source/Compiler/PipelineCompiler.cpp
    Layer/Source/Compiler/Diagnostic/DiagnosticPrettyPrint.cpp
    Layer/Source/Controllers/InstrumentationController.cpp
    Layer/Source/Controllers/FeatureController.cpp
    Layer/Source/Controllers/MetadataController.cpp
    Layer/Source/Controllers/VersioningController.cpp
    Layer/Source/Export/ShaderExportHost.cpp
    Layer/Source/Export/ShaderExportStreamAllocator.cpp
    Layer/Source/Export/ShaderExportDescriptorAllocator.cpp
    Layer/Source/Export/ShaderExportStreamer.cpp
    Layer/Source/Symbolizer/ShaderSGUIDHost.cpp
    Layer/Source/Scheduler/Scheduler.cpp
    Layer/Source/Compiler/SpvSourceMap.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockAnnotation.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockCapability.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockEntryPoint.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockDebugStringSource.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockFunction.cpp
    Layer/Source/Compiler/Blocks/SpvPhysicalBlockTypeConstantVariable.cpp
    Layer/Source/Compiler/Utils/SpvUtilShaderExport.cpp
    Layer/Source/Compiler/Utils/SpvUtilShaderPRMT.cpp
    Layer/Source/Compiler/Utils/SpvUtilShaderDescriptorConstantData.cpp
    Layer/Source/Compiler/Utils/SpvUtilShaderConstantData.cpp
    Layer/Source/Compiler/SpvPhysicalBlockScan.cpp
    Layer/Source/Compiler/SpvPhysicalBlockTable.cpp
    Layer/Source/ShaderData/ShaderDataHost.cpp
    Layer/Source/Resource/PhysicalResourceMappingTable.cpp
    Layer/Source/Resource/PhysicalResourceMappingTablePersistentVersion.cpp
    Layer/Source/Resource.cpp
    Layer/Source/ShaderProgram/ShaderProgramHost.cpp
    Layer/Source/Command/UserCommandBuffer.cpp
    Layer/Source/Debug.cpp

    # VMA implementation
    Layer/Source/VMA.cpp

    # Generated
    Layer/Source/CommandBuffer.Gen.cpp
    Layer/Source/DeepCopy.Gen.cpp

    # Generated dependency headers
    Layer/Include/Backends/Vulkan/DeepCopyObjects.Gen.h
    Layer/Include/Backends/Vulkan/CommandBufferDispatchTable.Gen.h
    Layer/Include/Backends/Vulkan/Compiler/Spv.Gen.h

    # Generated modules
    ${GeneratedInbuilt}
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.Vulkan.Layer CXX Layer)

# Link against backend
target_link_libraries(
    GRS.Backends.Vulkan.Layer PUBLIC
    GRS.Libraries.Backend
    GRS.Libraries.Bridge
    GRS.Libraries.Common

    # Win32
    $<$<PLATFORM_ID:Windows>:Dbghelp>
)

# Include directories
target_include_directories(
    GRS.Backends.Vulkan.Layer PUBLIC
    Layer/Include ${CMAKE_CURRENT_BINARY_DIR}/Layer/Include
)

# Setup dependencies
ExternalProject_Link(GRS.Backends.Vulkan.Layer VulkanHeaders)
ExternalProject_Link(GRS.Backends.Vulkan.Layer VMA)

# Validation dependencies
ExternalProject_Link(GRS.Backends.Vulkan.Layer SPIRVTools SPIRV-Tools)

# Copy layer definition (configuration time)
ConfigureOutput(Layer/Resources/VK_LAYER_GPUOPEN_GRS.json VK_LAYER_GPUOPEN_GRS.json)

#----- Discovery -----#

# Create feature
add_library(
    GRS.Backends.Vulkan.Discovery SHARED
    Discovery/Source/Plugin.cpp
    Discovery/Source/VulkanDiscoveryListener.cpp
    Vulkan.xml
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.Vulkan.Discovery CXX Discovery)

# Includes
target_include_directories(GRS.Backends.Vulkan.Discovery PUBLIC Discovery/Include)

# Links
target_link_libraries(GRS.Backends.Vulkan.Discovery PUBLIC GRS.Libraries.Discovery GRS.Backends.Vulkan.Layer)

# Set all outputs to Plugins/.
PostBuildCopy(GRS.Backends.Vulkan.Discovery Plugins)

# Copy spec xml
ConfigureOutput(Vulkan.xml Plugins/Vulkan.xml)

#----- Test Device -----#

# Create test layer
add_library(
    GRS.Backends.Vulkan.TestDevice STATIC
    TestDevice/Source/Device.cpp
    TestDevice/Source/VMA.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.Vulkan.TestDevice CXX TestDevice)

# Include
target_include_directories(GRS.Backends.Vulkan.TestDevice PUBLIC TestDevice/Include)

# Link
target_link_libraries(GRS.Backends.Vulkan.TestDevice PUBLIC GRS.Backends.Vulkan.Layer GRS.Test.Device)

# Setup dependencies
ExternalProject_Link(GRS.Backends.Vulkan.TestDevice VulkanHeaders)
ExternalProject_Link(GRS.Backends.Vulkan.TestDevice VMA)

#----- Test Layer -----#

# Create test layer
add_library(
    GRS.Backends.Vulkan.Tests.UserDataLayer SHARED
    Tests/Layer/UserDataLayer.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.Vulkan.Tests.UserDataLayer CXX Tests/Layer)

# Setup dependencies
ExternalProject_Link(GRS.Backends.Vulkan.Tests.UserDataLayer VulkanHeaders)

# Copy layer definition (configuration time)
ConfigureOutput(Tests/Resources/VK_GPUOpen_Test_UserDataLayer.json VK_GPUOpen_Test_UserDataLayer.json)

#----- Test Application -----#

# Hlsl files
Project_AddHLSL(GeneratedTest cs_6_0 "-Od" Tests/Data/WriteUAV.hlsl Tests/Include/Data/WriteUAV kSPIRVWriteUAV)
Project_AddHLSL(GeneratedTest cs_6_0 "-Od" Tests/Data/WriteUAVNegative.hlsl Tests/Include/Data/WriteUAVNegative kSPIRVWriteUAVNegative)

# Generate the schema
Project_AddShaderSchema(GeneratedTestSchema Tests/Schemas/WritingNegativeValue.xml Tests/Include/Schemas)

# Create test app
add_executable(
    GRS.Backends.Vulkan.Tests
    Tests/Source/Main.cpp
    Tests/Source/Loader.cpp
    Tests/Source/UserData.cpp
    Tests/Source/Layer/Layer.cpp
    Tests/Source/Layer/OffsetStoresByOne.cpp
    Tests/Source/Layer/WritingNegativeValue.cpp
    Tests/Source/VMA.cpp

    # Generated
    ${GeneratedTest}
    ${GeneratedTestSchemaCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.Vulkan.Tests CXX Tests)

# Includes
target_include_directories(GRS.Backends.Vulkan.Tests PUBLIC Layer/Include Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Links
target_link_libraries(GRS.Backends.Vulkan.Tests PUBLIC GRS.Libraries.Common GRS.Backends.Vulkan.Layer)

# Setup dependencies
ExternalProject_Link(GRS.Backends.Vulkan.Tests Catch2)
ExternalProject_Link(GRS.Backends.Vulkan.Tests VMA)
ExternalProject_Link(GRS.Backends.Vulkan.Tests VulkanLoader vulkan-1)

# Compiler definitions
target_compile_definitions(
    GRS.Backends.Vulkan.Tests PRIVATE
    CATCH_CONFIG_ENABLE_BENCHMARKING # Enable benchmarking
)

# Add dependency on the layer
add_dependencies(
    GRS.Backends.Vulkan.Tests
    GRS.Backends.Vulkan.Tests.UserDataLayer
)
