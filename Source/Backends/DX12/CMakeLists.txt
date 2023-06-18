# 
# The MIT License (MIT)
# 
# Copyright (c) 2023 Miguel Petersen
# Copyright (c) 2023 Advanced Micro Devices, Inc
# Copyright (c) 2023 Avalanche Studios Group
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

#----- Config -----#

set(DX12TableType OBJECTWRAPPER)

#----- Generator -----#

# Create generator
add_executable(
    GRS.Backends.DX12.Generator
    Generator/Source/Generator.cpp
    Generator/Source/Specification.cpp
    Generator/Source/Detour.cpp
    Generator/Source/Wrappers.cpp
    Generator/Source/WrappersImpl.cpp
    Generator/Source/ObjectWrappers.cpp
    Generator/Source/VTable.cpp
    Generator/Source/Table.cpp
    Generator/Source/DeepCopy.cpp
    Generator/Source/DeepCopyImpl.cpp
    Generator/Source/DXILTables.cpp
    Generator/Source/DXILIntrinsics.cpp
    Generator/Source/FeatureProxies.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.DX12.Generator CXX Generator)

# Local links
target_link_libraries(GRS.Backends.DX12.Generator PUBLIC GRS.Libraries.Common)

# Private includes
target_include_directories(
    GRS.Backends.DX12.Generator PRIVATE
    Generator/Include
)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Backends.DX12.Generator PRIVATE /EHa)
endif()

# Setup dependencies
ExternalProject_Link(GRS.Backends.DX12.Generator ArgParse)
ExternalProject_Link(GRS.Backends.DX12.Generator TinyXML2 tinyxml2)
ExternalProject_Link(GRS.Backends.DX12.Generator JSON)
ExternalProject_Link(GRS.Backends.DX12.Generator LibClang libclang)

#----- Layer -----#

# Paths
set(Specification                      ${CMAKE_CURRENT_BINARY_DIR}/Generator/Specification.json)
set(SpecificationTemplate              ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Specification.json)
set(DetourTemplate                     ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Detour.ht)
set(WrappersTemplate                   ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Wrappers.ht)
set(WrappersImplTemplate               ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Wrappers.cppt)
set(ObjectWrappersImplTemplate         ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/ObjectWrappers.cppt)
set(DeepCopyTemplate                   ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopy.ht)
set(DeepCopyImplTemplate               ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopy.cppt)
set(TableTemplate                      ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Table.ht)
set(VTableTemplate                     ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/VTable.cppt)
set(DXILTemplate                       ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DXIL.ht)
set(DXILIntrinsicsTemplate             ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DXILIntrinsics.ht)
set(FeatureProxiesTemplate             ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/FeatureProxies.ht)

# Resource Paths
set(HooksSpec    ${CMAKE_CURRENT_SOURCE_DIR}/Layer/Resources/Hooks.json)
set(DeepCopySpec ${CMAKE_CURRENT_SOURCE_DIR}/Layer/Resources/DeepCopy.json)
set(DXILRST      ${CMAKE_SOURCE_DIR}/ThirdParty/DXIL/DXIL.rst)

# Generate the DXIL tables
add_custom_command(
    OUTPUT Layer/Include/Backends/DX12/Compiler/DXIL/DXIL.Gen.h
    DEPENDS GRS.Backends.DX12.Generator
    DEPENDS
        ${DXILTemplate}
        ${DXILRST}
    COMMAND GRS.Backends.DX12.Generator
        -template ${DXILTemplate}
        -dxilrst ${DXILRST}
        -hooksjson ${HooksSpec}
        -gentype dxiltables
        -o Layer/Include/Backends/DX12/Compiler/DXIL/DXIL.Gen.h
)

# Generate the DXIL intrinsics
add_custom_command(
    OUTPUT Layer/Include/Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsics.Gen.h
    DEPENDS GRS.Backends.DX12.Generator
    DEPENDS
        ${DXILIntrinsicsTemplate}
        ${DXILRST}
    COMMAND GRS.Backends.DX12.Generator
        -template ${DXILIntrinsicsTemplate}
        -dxilrst ${DXILRST}
        -hooksjson ${HooksSpec}
        -gentype dxilintrinsics
        -o Layer/Include/Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsics.Gen.h
)

# Generate the specification
add_custom_command(
    OUTPUT ${Specification}
    DEPENDS GRS.Backends.DX12.Generator
    DEPENDS
        ${SpecificationTemplate}
        ${HooksSpec}
    COMMAND GRS.Backends.DX12.Generator
        -template ${SpecificationTemplate}
        -gentype specification
        -hooksjson ${HooksSpec}
        -d3d12h ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Resources/D3D12Specification.h
        -o ${Specification}
)

# Generate the feature proxies
add_custom_command(
    OUTPUT Layer/Include/Backends/DX12/FeatureProxies.Gen.h
    DEPENDS GRS.Backends.DX12.Generator
    DEPENDS
        ${FeatureProxiesTemplate}
        ${Specification}
        ${DXILRST}
    COMMAND GRS.Backends.DX12.Generator
        -template ${FeatureProxiesTemplate}
        -dxilrst ${DXILRST}
        -specjson ${Specification}
        -hooksjson ${HooksSpec}
        -gentype featureproxies
        -o Layer/Include/Backends/DX12/FeatureProxies.Gen.h
)

# Generate the detour header
add_custom_command(
    OUTPUT Layer/Include/Backends/DX12/Detour.Gen.h
    DEPENDS GRS.Backends.DX12.Generator
    DEPENDS
        ${DetourTemplate}
        ${Specification}
        ${HooksSpec}
    COMMAND GRS.Backends.DX12.Generator
        -template ${DetourTemplate}
        -specjson ${Specification}
        -hooksjson ${HooksSpec}
        -gentype detour
        -o Layer/Include/Backends/DX12/Detour.Gen.h
)

# Generate the deep copy header
add_custom_command(
    OUTPUT
        Layer/Include/Backends/DX12/DeepCopy.Gen.h
    DEPENDS GRS.Backends.DX12.Generator
    DEPENDS
        ${DeepCopyTemplate}
        ${Specification}
        ${DeepCopySpec}
    COMMAND GRS.Backends.DX12.Generator
        -template ${DeepCopyTemplate}
        -specjson ${Specification}
        -deepcopyjson ${DeepCopySpec}
        -gentype deepcopy
        -o Layer/Include/Backends/DX12/DeepCopy.Gen.h
)

# Generate the deep copy source
add_custom_command(
    OUTPUT
        Layer/Source/DeepCopy.Gen.cpp
    DEPENDS GRS.Backends.DX12.Generator
    DEPENDS
        ${DeepCopyImplTemplate}
        ${Specification}
        ${DeepCopySpec}
    COMMAND GRS.Backends.DX12.Generator
        -template ${DeepCopyImplTemplate}
        -specjson ${Specification}
        -deepcopyjson ${DeepCopySpec}
        -gentype deepcopyimpl
        -o Layer/Source/DeepCopy.Gen.cpp
)

# Handle table type
if (${DX12TableType} STREQUAL OBJECTWRAPPER)
    # Generate the detour header
    add_custom_command(
        OUTPUT
            Layer/Source/ObjectWrappers.Gen.cpp
        DEPENDS GRS.Backends.DX12.Generator
        DEPENDS
            ${ObjectWrappersImplTemplate}
            ${Specification}
            ${HooksSpec}
        COMMAND GRS.Backends.DX12.Generator
            -template ${ObjectWrappersImplTemplate}
            -specjson ${Specification}
            -hooksjson ${HooksSpec}
            -gentype objectwrappers
            -o Layer/Source/ObjectWrappers.Gen.cpp
    )

    set(GeneratedSources Layer/Source/ObjectWrappers.Gen.cpp)
elseif (${DX12TableType} STREQUAL WRAPPER)
    # Generate the detour header
    add_custom_command(
        OUTPUT
            Layer/Include/Backends/DX12/Wrappers.Gen.h
        DEPENDS GRS.Backends.DX12.Generator
        DEPENDS
            ${WrappersTemplate}
            ${Specification}
            ${HooksSpec}
        COMMAND GRS.Backends.DX12.Generator
            -template ${WrappersTemplate}
            -specjson ${Specification}
            -hooksjson ${HooksSpec}
            -gentype wrappers
            -o Layer/Include/Backends/DX12/Wrappers.Gen.h
    )

    # Generate the detour header
    add_custom_command(
        OUTPUT
            Layer/Source/Wrappers.Gen.cpp
        DEPENDS GRS.Backends.DX12.Generator
        DEPENDS
            ${WrappersImplTemplate}
            ${Specification}
            ${HooksSpec}
        COMMAND GRS.Backends.DX12.Generator
            -template ${WrappersImplTemplate}
            -specjson ${Specification}
            -hooksjson ${HooksSpec}
            -gentype wrappersimpl
            -o Layer/Source/Wrappers.Gen.cpp
    )

    set(GeneratedSources Layer/Include/Backends/DX12/Wrappers.Gen.h Layer/Source/Wrappers.Gen.cpp)
elseif (${DX12TableType} STREQUAL VIRTUAL)
    # Generate the detour header
    add_custom_command(
        OUTPUT
            Layer/Source/VTable.Gen.cpp
        DEPENDS GRS.Backends.DX12.Generator
        DEPENDS
            ${VTableTemplate}
            ${Specification}
            ${HooksSpec}
        COMMAND GRS.Backends.DX12.Generator
            -template ${VTableTemplate}
            -specjson ${Specification}
            -hooksjson ${HooksSpec}
            -gentype vtable
            -o Layer/Source/VTable.Gen.cpp
    )

    set(GeneratedSources Layer/Source/VTable.Gen.cpp)
endif()

# Generate the detour header
add_custom_command(
    OUTPUT
        Layer/Include/Backends/DX12/Table.Gen.h
    DEPENDS GRS.Backends.DX12.Generator
    DEPENDS
        ${TableTemplate}
        ${Specification}
        ${HooksSpec}
    COMMAND GRS.Backends.DX12.Generator
        -template ${TableTemplate}
        -specjson ${Specification}
        -hooksjson ${HooksSpec}
        -gentype table
        -o Layer/Include/Backends/DX12/Table.Gen.h
)

# Inbuilt modules
Project_AddHLSL(GeneratedInbuilt cs_6_0 "-Od" Layer/Modules/InbuiltTemplateModule.hlsl Layer/Include/Backends/DX12/Modules/InbuiltTemplateModule kSPIRVInbuiltTemplateModule)

# Create layer
add_library(
    GRS.Backends.DX12.Layer SHARED
    Layer/Source/DXGIFactory.cpp
    Layer/Source/Device.cpp
    Layer/Source/Resource.cpp
    Layer/Source/Fence.cpp
    Layer/Source/CommandList.cpp
    Layer/Source/RootSignature.cpp
    Layer/Source/DescriptorHeap.cpp
    Layer/Source/Pipeline.cpp
    Layer/Source/PipelineLibrary.cpp
    Layer/Source/SwapChain.cpp
    Layer/Source/DLL.cpp
    Layer/Source/Layer.cpp
    Layer/Source/Compiler/ShaderCompiler.cpp
    Layer/Source/Compiler/ShaderCompilerDebug.cpp
    Layer/Source/Compiler/PipelineCompiler.cpp
    Layer/Source/Compiler/DXModule.cpp
    Layer/Source/Compiler/DXBC/DXBCModule.cpp
    Layer/Source/Compiler/DXBC/DXBCPhysicalBlockScan.cpp
    Layer/Source/Compiler/DXBC/DXBCPhysicalBlockTable.cpp
    Layer/Source/Compiler/DXBC/DXBCUtils.cpp
    Layer/Source/Compiler/DXBC/Blocks/DXBCPhysicalBlockShader.cpp
    Layer/Source/Compiler/DXBC/Blocks/DXBCPhysicalBlockPipelineStateValidation.cpp
    Layer/Source/Compiler/DXBC/Blocks/DXBCPhysicalBlockRootSignature.cpp
    Layer/Source/Compiler/DXBC/Blocks/DXBCPhysicalBlockInputSignature.cpp
    Layer/Source/Compiler/DXBC/Blocks/DXBCPhysicalBlockFeatureInfo.cpp
    Layer/Source/Compiler/DXBC/Blocks/DXBCPhysicalBlockDebug.cpp
    Layer/Source/Compiler/DXBC/DXBCSigner.cpp
    Layer/Source/Compiler/DXIL/DXILModule.cpp
    Layer/Source/Compiler/DXIL/DXILPhysicalBlockScan.cpp
    Layer/Source/Compiler/DXIL/DXILPhysicalBlockTable.cpp
    Layer/Source/Compiler/DXIL/Blocks/DXILPhysicalBlockFunction.cpp
    Layer/Source/Compiler/DXIL/Blocks/DXILPhysicalBlockFunctionAttribute.cpp
    Layer/Source/Compiler/DXIL/Blocks/DXILPhysicalBlockString.cpp
    Layer/Source/Compiler/DXIL/Blocks/DXILPhysicalBlockMetadata.cpp
    Layer/Source/Compiler/DXIL/Blocks/DXILPhysicalBlockSymbol.cpp
    Layer/Source/Compiler/DXIL/Blocks/DXILPhysicalBlockType.cpp
    Layer/Source/Compiler/DXIL/Blocks/DXILPhysicalBlockGlobal.cpp
    Layer/Source/Compiler/DXIL/Utils/DXILUtilIntrinsics.cpp
    Layer/Source/Compiler/DXIL/LLVM/LLVMPrettyPrint.cpp
    Layer/Source/Compiler/DXIL/DXILSigner.cpp
    Layer/Source/Compiler/DXIL/DXILDebugModule.cpp
    Layer/Source/Controllers/InstrumentationController.cpp
    Layer/Source/Controllers/MetadataController.cpp
    Layer/Source/Controllers/VersioningController.cpp
    Layer/Source/Controllers/FeatureController.cpp
    Layer/Source/Controllers/PDBController.cpp
    Layer/Source/Export/ExportHost.cpp
    Layer/Source/Export/ShaderExportDescriptorAllocator.cpp
    Layer/Source/Export/ShaderExportStreamer.cpp
    Layer/Source/Export/ShaderExportStreamAllocator.cpp
    Layer/Source/Export/ShaderExportDescriptorLayout.cpp
    Layer/Source/Export/ShaderExportConstantAllocator.cpp
    Layer/Source/Symbolizer/ShaderSGUIDHost.cpp
    Layer/Source/Allocation/DeviceAllocator.cpp
    Layer/Source/Resource/PhysicalResourceMappingTable.cpp
    Layer/Source/ShaderData/ShaderDataHost.cpp
    Layer/Source/Scheduler/Scheduler.cpp
    Layer/Source/IncrementalFence.cpp
    Layer/Source/ShaderProgram/ShaderProgramHost.cpp
    Layer/Source/Command/UserCommandBuffer.cpp
    Layer/Source/FeatureProxies.cpp

    # Generated source dependencies
    Layer/Source/DeepCopy.Gen.cpp
    ${GeneratedSources}

    # Generated header dependencies
    Layer/Include/Backends/DX12/Compiler/DXIL/DXIL.Gen.h
    Layer/Include/Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsics.Gen.h
    Layer/Include/Backends/DX12/FeatureProxies.Gen.h
    Layer/Include/Backends/DX12/Detour.Gen.h
    Layer/Include/Backends/DX12/Table.Gen.h
    Layer/Include/Backends/DX12/DeepCopy.Gen.h

    # Generated modules
    ${GeneratedInbuilt}
)

if (${DX12TableType} STREQUAL WRAPPER)
    # Disable stack cookies, messes with hooking method on wide returns
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
        # Native MSVC compiler
        set(NoCookies "/GS-")
    elseif ("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" MATCHES "MSVC")
        # Supports MSVC arguments (like clang-cl)
        set(NoCookies "/GS-")
    else()
        # GNU like
        set(NoCookies "-fno-stack-protector")
    endif()

    # Set on file
    set_source_files_properties(Layer/Source/Wrappers.Gen.cpp PROPERTIES COMPILE_FLAGS ${NoCookies})
endif()

# Internal definitions
target_compile_definitions(GRS.Backends.DX12.Layer PRIVATE DX12_PRIVATE=1)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.DX12.Layer CXX Layer)

# Link against backend
target_link_libraries(
    GRS.Backends.DX12.Layer PUBLIC
    GRS.Libraries.Backend
    GRS.Libraries.Bridge
    GRS.Libraries.Common

    # d3d12
    d3d12.lib
    dxgi.lib

    # Generated source
)

# Setup dependencies
ExternalProject_Link(GRS.Backends.DX12.Layer Detour Detour)
ExternalProject_Link(GRS.Backends.DX12.Layer DxbcSigner)
ExternalProject_Link(GRS.Backends.DX12.Layer D3D12MA $<$<CONFIG:Debug>:D3D12MAd> $<$<CONFIG:Release>:D3D12MAr> $<$<CONFIG:RelWithDebInfo>:D3D12MArd>)

# Include directories
target_include_directories(
    GRS.Backends.DX12.Layer PUBLIC
    Layer/Include ${CMAKE_CURRENT_BINARY_DIR}/Layer/Include
)

#----- Bootstrapper -----#

# Create service
add_library(
    GRS.Backends.DX12.Bootstrapper SHARED
    Bootstrapper/Source/DLL.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.DX12.Bootstrapper CXX Bootstrapper)

# Include directories
target_include_directories(GRS.Backends.DX12.Bootstrapper PUBLIC Layer/Include Bootstrapper/Include)

# Dependencies
target_link_libraries(GRS.Backends.DX12.Bootstrapper PUBLIC GRS.Libraries.Common)

# External dependencies
ExternalProject_Link(GRS.Backends.DX12.Bootstrapper Detour Detour)

# Binary dependencies
add_dependencies(GRS.Backends.DX12.Bootstrapper GRS.Backends.DX12.Layer)

#----- Service -----#

# Create service
add_executable(
    GRS.Backends.DX12.Service
    Service/Source/main.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.DX12.Service CXX Service)

# Include directories
target_include_directories(GRS.Backends.DX12.Service PUBLIC Service/Include)

# Dependencies
target_link_libraries(GRS.Backends.DX12.Service PUBLIC GRS.Libraries.Common)

# Binary dependencies
add_dependencies(GRS.Backends.DX12.Service GRS.Backends.DX12.Bootstrapper)

#----- Discovery -----#

# Create feature
add_library(
    GRS.Backends.DX12.Discovery SHARED
    Discovery/Source/Plugin.cpp
    Discovery/Source/DX12DiscoveryListener.cpp
    DX12.xml
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.DX12.Discovery CXX Discovery)

# Includes
target_include_directories(GRS.Backends.DX12.Discovery PUBLIC Discovery/Include Service/Include)

# Links
target_link_libraries(GRS.Backends.DX12.Discovery PUBLIC GRS.Libraries.Discovery GRS.Backends.DX12.Layer)

# Set all outputs to Plugins/.
PostBuildCopy(GRS.Backends.DX12.Discovery Plugins)

# Copy spec xml
ConfigureOutput(DX12.xml Plugins/DX12.xml)

#----- Test Device -----#

# Create test layer
add_library(
    GRS.Backends.DX12.TestDevice STATIC
    TestDevice/Source/Device.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.DX12.TestDevice CXX TestDevice)

# Include
target_include_directories(GRS.Backends.DX12.TestDevice PUBLIC TestDevice/Include)

# Link
target_link_libraries(GRS.Backends.DX12.TestDevice PUBLIC GRS.Backends.DX12.Layer GRS.Test.Device)

#----- Test Application -----#

# Hlsl files
set(Generated "")
Project_AddHLSL(Generated vs_6_0 "-Od -E VSMain" Tests/Data/HelloTriangle.hlsl Tests/Include/Data/HelloTriangleVS kHelloTriangleVS)
Project_AddHLSL(Generated ps_6_0 "-Od -E PSMain" Tests/Data/HelloTriangle.hlsl Tests/Include/Data/HelloTrianglePS kHelloTrianglePS)

# Create test app
add_executable(
    GRS.Backends.DX12.Tests
    Tests/Source/Main.cpp
    Tests/Source/HelloTriangle.cpp
    Tests/Source/WrappingBenchmark.cpp

    # Pull generated
    ${Generated}
)

# IDE source discovery
SetSourceDiscovery(GRS.Backends.DX12.Tests CXX Tests)

# Includes
target_include_directories(GRS.Backends.DX12.Tests PUBLIC Layer/Include Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Links
target_link_libraries(GRS.Backends.DX12.Tests PUBLIC GRS.Libraries.Common GRS.Backends.DX12.Layer)

# Setup dependencies
ExternalProject_Link(GRS.Backends.DX12.Tests Catch2)

# Compiler definitions
target_compile_definitions(
    GRS.Backends.DX12.Tests PRIVATE
    CATCH_CONFIG_ENABLE_BENCHMARKING # Enable benchmarking
)
