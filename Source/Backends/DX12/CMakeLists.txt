#----- Config -----#

set(DX12TableType WRAPPER)

#----- Generator -----#

# Create generator
add_executable(
    Backends.DX12.Generator
    Generator/Source/Generator.cpp
    Generator/Source/Specification.cpp
    Generator/Source/Detour.cpp
    Generator/Source/Wrappers.cpp
    Generator/Source/WrappersImpl.cpp
    Generator/Source/VTable.cpp
    Generator/Source/Table.cpp
    Generator/Source/DeepCopy.cpp
    Generator/Source/DeepCopyImpl.cpp
    Generator/Source/DXILTables.cpp
    Generator/Source/DXILIntrinsics.cpp
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Generator CXX Generator)

# Local links
target_link_libraries(Backends.DX12.Generator PUBLIC Libraries.Common)

# Private includes
target_include_directories(
    Backends.DX12.Generator PRIVATE
    Generator/Include
)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(Backends.DX12.Generator PRIVATE /EHa)
endif()

# Setup dependencies
ExternalProject_Link(Backends.DX12.Generator ArgParse)
ExternalProject_Link(Backends.DX12.Generator TinyXML2 tinyxml2)
ExternalProject_Link(Backends.DX12.Generator JSON)
ExternalProject_Link(Backends.DX12.Generator LibClang libclang)

#----- Layer -----#

# Paths
set(Specification                      ${CMAKE_CURRENT_BINARY_DIR}/Generator/Specification.json)
set(SpecificationTemplate              ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Specification.json)
set(DetourTemplate                     ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Detour.ht)
set(WrappersTemplate                   ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Wrappers.ht)
set(WrappersImplTemplate               ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Wrappers.cppt)
set(DeepCopyTemplate                   ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopy.ht)
set(DeepCopyImplTemplate               ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DeepCopy.cppt)
set(TableTemplate                      ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Table.ht)
set(VTableTemplate                     ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/VTable.cppt)
set(DXILTemplate                       ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DXIL.ht)
set(DXILIntrinsicsTemplate             ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/DXILIntrinsics.ht)

# Resource Paths
set(HooksSpec    ${CMAKE_CURRENT_SOURCE_DIR}/Layer/Resources/Hooks.json)
set(DeepCopySpec ${CMAKE_CURRENT_SOURCE_DIR}/Layer/Resources/DeepCopy.json)
set(DXILRST      ${CMAKE_SOURCE_DIR}/ThirdParty/DXIL/DXIL.rst)

# Generate the DXIL tables
add_custom_command(
    OUTPUT Layer/Include/Backends/DX12/Compiler/DXIL/DXIL.Gen.h
    DEPENDS Backends.DX12.Generator
    DEPENDS
        ${DXILTemplate}
        ${DXILRST}
    COMMAND Backends.DX12.Generator
        -template ${DXILTemplate}
        -dxilrst ${DXILRST}
        -hooksjson ${HooksSpec}
        -gentype dxiltables
        -o Layer/Include/Backends/DX12/Compiler/DXIL/DXIL.Gen.h
)

# Generate the DXIL intrinsics
add_custom_command(
    OUTPUT Layer/Include/Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsics.Gen.h
    DEPENDS Backends.DX12.Generator
    DEPENDS
        ${DXILIntrinsicsTemplate}
        ${DXILRST}
    COMMAND Backends.DX12.Generator
        -template ${DXILIntrinsicsTemplate}
        -dxilrst ${DXILRST}
        -hooksjson ${HooksSpec}
        -gentype dxilintrinsics
        -o Layer/Include/Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsics.Gen.h
)

# Generate the specification
add_custom_command(
    OUTPUT ${Specification}
    DEPENDS Backends.DX12.Generator
    DEPENDS
        ${SpecificationTemplate}
        ${HooksSpec}
    COMMAND Backends.DX12.Generator
        -template ${SpecificationTemplate}
        -gentype specification
        -hooksjson ${HooksSpec}
        -d3d12h ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Resources/D3D12Specification.h
        -o ${Specification}
)

# Generate the detour header
add_custom_command(
    OUTPUT Layer/Include/Backends/DX12/Detour.Gen.h
    DEPENDS Backends.DX12.Generator
    DEPENDS
        ${DetourTemplate}
        ${Specification}
        ${HooksSpec}
    COMMAND Backends.DX12.Generator
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
    DEPENDS Backends.DX12.Generator
    DEPENDS
        ${DeepCopyTemplate}
        ${Specification}
        ${DeepCopySpec}
    COMMAND Backends.DX12.Generator
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
    DEPENDS Backends.DX12.Generator
    DEPENDS
        ${DeepCopyImplTemplate}
        ${Specification}
        ${DeepCopySpec}
    COMMAND Backends.DX12.Generator
        -template ${DeepCopyImplTemplate}
        -specjson ${Specification}
        -deepcopyjson ${DeepCopySpec}
        -gentype deepcopyimpl
        -o Layer/Source/DeepCopy.Gen.cpp
)

# Handle table type
if (${DX12TableType} STREQUAL WRAPPER)
    # Generate the detour header
    add_custom_command(
        OUTPUT
            Layer/Include/Backends/DX12/Wrappers.Gen.h
        DEPENDS Backends.DX12.Generator
        DEPENDS
            ${WrappersTemplate}
            ${Specification}
            ${HooksSpec}
        COMMAND Backends.DX12.Generator
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
        DEPENDS Backends.DX12.Generator
        DEPENDS
            ${WrappersImplTemplate}
            ${Specification}
            ${HooksSpec}
        COMMAND Backends.DX12.Generator
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
        DEPENDS Backends.DX12.Generator
        DEPENDS
            ${VTableTemplate}
            ${Specification}
            ${HooksSpec}
        COMMAND Backends.DX12.Generator
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
    DEPENDS Backends.DX12.Generator
    DEPENDS
        ${TableTemplate}
        ${Specification}
        ${HooksSpec}
    COMMAND Backends.DX12.Generator
        -template ${TableTemplate}
        -specjson ${Specification}
        -hooksjson ${HooksSpec}
        -gentype table
        -o Layer/Include/Backends/DX12/Table.Gen.h
)

# Create layer
add_library(
    Backends.DX12.Layer SHARED
    Layer/Source/DXGIFactory.cpp
    Layer/Source/Device.cpp
    Layer/Source/Resource.cpp
    Layer/Source/Fence.cpp
    Layer/Source/CommandList.cpp
    Layer/Source/RootSignature.cpp
    Layer/Source/DescriptorHeap.cpp
    Layer/Source/Pipeline.cpp
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
    Layer/Source/Compiler/DXBC/Blocks/DXBCPhysicalBlockShader.cpp
    Layer/Source/Compiler/DXBC/Blocks/DXBCPhysicalBlockPipelineStateValidation.cpp
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
    Layer/Source/Export/ExportHost.cpp
    Layer/Source/Export/ShaderExportDescriptorAllocator.cpp
    Layer/Source/Export/ShaderExportStreamer.cpp
    Layer/Source/Export/ShaderExportStreamAllocator.cpp
    Layer/Source/Symbolizer/ShaderSGUIDHost.cpp
    Layer/Source/Allocation/DeviceAllocator.cpp
    Layer/Source/IncrementalFence.cpp

    # Generated source dependencies
    Layer/Source/DeepCopy.Gen.cpp
    ${GeneratedSources}

    # Generated header dependencies
    Layer/Include/Backends/DX12/Compiler/DXIL/DXIL.Gen.h
    Layer/Include/Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsics.Gen.h
    Layer/Include/Backends/DX12/Detour.Gen.h
    Layer/Include/Backends/DX12/Table.Gen.h
    Layer/Include/Backends/DX12/DeepCopy.Gen.h
)

# Internal definitions
target_compile_definitions(Backends.DX12.Layer PRIVATE DX12_PRIVATE=1)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Layer CXX Layer)

# Link against backend
target_link_libraries(
    Backends.DX12.Layer PUBLIC
    Libraries.Backend
    Libraries.Bridge
    Libraries.Common

    # d3d12
    d3d12.lib
    dxgi.lib

    # Generated source
)

# Setup dependencies
ExternalProject_Link(Backends.DX12.Layer Detour Detour)
ExternalProject_Link(Backends.DX12.Layer DxbcSigner DxbcSigner)
ExternalProject_Link(Backends.DX12.Layer D3D12MA $<$<CONFIG:Debug>:D3D12MAd> $<$<CONFIG:Release>:D3D12MAr>)

# Include directories
target_include_directories(
    Backends.DX12.Layer PUBLIC
    Layer/Include ${CMAKE_CURRENT_BINARY_DIR}/Layer/Include
)

#----- Bootstrapper -----#

# Create service
add_library(
    Backends.DX12.Bootstrapper SHARED
    Bootstrapper/Source/DLL.cpp
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Bootstrapper CXX Bootstrapper)

# Include directories
target_include_directories(Backends.DX12.Bootstrapper PUBLIC Bootstrapper/Include)

# Dependencies
target_link_libraries(Backends.DX12.Bootstrapper PUBLIC Libraries.Common)

# External dependencies
ExternalProject_Link(Backends.DX12.Bootstrapper Detour Detour)

# Binary dependencies
add_dependencies(Backends.DX12.Bootstrapper Backends.DX12.Layer)

#----- Service -----#

# Create service
add_executable(
    Backends.DX12.Service
    Service/Source/main.cpp
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Service CXX Service)

# Include directories
target_include_directories(Backends.DX12.Service PUBLIC Service/Include)

# Dependencies
target_link_libraries(Backends.DX12.Service PUBLIC Libraries.Common)

# Binary dependencies
add_dependencies(Backends.DX12.Service Backends.DX12.Bootstrapper)

#----- Discovery -----#

# Create feature
add_library(
    Backends.DX12.Discovery SHARED
    Discovery/Source/Plugin.cpp
    Discovery/Source/DX12DiscoveryListener.cpp
    DX12.xml
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Discovery CXX Discovery)

# Includes
target_include_directories(Backends.DX12.Discovery PUBLIC Discovery/Include Service/Include)

# Links
target_link_libraries(Backends.DX12.Discovery PUBLIC Libraries.Discovery Backends.DX12.Layer)

# Set all outputs to Plugins/.
PostBuildCopy(Backends.DX12.Discovery Plugins)

# Copy spec xml
ConfigureOutput(DX12.xml Plugins/DX12.xml)

#----- Test Device -----#

# Create test layer
add_library(
    Backends.DX12.TestDevice STATIC
    TestDevice/Source/Device.cpp
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.TestDevice CXX TestDevice)

# Include
target_include_directories(Backends.DX12.TestDevice PUBLIC TestDevice/Include)

# Link
target_link_libraries(Backends.DX12.TestDevice PUBLIC Backends.DX12.Layer Test.Device)

#----- Test Application -----#

# Hlsl files
set(Generated "")
Project_AddHLSL(Generated vs_6_0 "-Od -E VSMain" Tests/Data/HelloTriangle.hlsl Tests/Include/Data/HelloTriangleVS kHelloTriangleVS)
Project_AddHLSL(Generated ps_6_0 "-Od -E PSMain" Tests/Data/HelloTriangle.hlsl Tests/Include/Data/HelloTrianglePS kHelloTrianglePS)

# Create test app
add_executable(
    Backends.DX12.Tests
    Tests/Source/Main.cpp
    Tests/Source/HelloTriangle.cpp
    Tests/Source/WrappingBenchmark.cpp

    # Pull generated
    ${Generated}
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Tests CXX Tests)

# Includes
target_include_directories(Backends.DX12.Tests PUBLIC Layer/Include Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Links
target_link_libraries(Backends.DX12.Tests PUBLIC Libraries.Common Backends.DX12.Layer)

# Setup dependencies
ExternalProject_Link(Backends.DX12.Tests Catch2)

# Compiler definitions
target_compile_definitions(
    Backends.DX12.Tests PRIVATE
    CATCH_CONFIG_ENABLE_BENCHMARKING # Enable benchmarking
)
