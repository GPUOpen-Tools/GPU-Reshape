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
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Generator Generator)

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

# Resource Paths
set(HooksSpec ${CMAKE_CURRENT_SOURCE_DIR}/Layer/Resources/Hooks.json)
set(DeepCopySpec ${CMAKE_CURRENT_SOURCE_DIR}/Layer/Resources/DeepCopy.json)

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
    Layer/Source/Pipeline.cpp
    Layer/Source/SwapChain.cpp
    Layer/Source/DLL.cpp

    # Generated source dependencies
    Layer/Source/DeepCopy.Gen.cpp
    ${GeneratedSources}

    # Generated header dependencies
    Layer/Include/Backends/DX12/Detour.Gen.h
    Layer/Include/Backends/DX12/Table.Gen.h
    Layer/Include/Backends/DX12/DeepCopy.Gen.h
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Layer Layer)

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

# Include directories
target_include_directories(
    Backends.DX12.Layer PUBLIC
    Layer/Include ${CMAKE_CURRENT_BINARY_DIR}/Layer/Include
)

#----- Discovery -----#

# Create feature
add_library(
    Backends.DX12.Discovery SHARED
    Discovery/Source/Plugin.cpp
    Discovery/Source/DX12DiscoveryListener.cpp
    DX12.xml
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Discovery Discovery)

# Includes
target_include_directories(Backends.DX12.Discovery PUBLIC Discovery/Include)

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
SetSourceDiscovery(Backends.DX12.TestDevice TestDevice)

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

    # Pull generated
    ${Generated}
)

# IDE source discovery
SetSourceDiscovery(Backends.DX12.Tests Tests)

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
