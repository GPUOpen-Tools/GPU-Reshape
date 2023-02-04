#----- Generator -----#

add_executable(
    GRS.Test.Device.Generator
    Generator/Source/Parser.cpp
    Generator/Source/Assembler.cpp
    Generator/Source/main.cpp
)

# Enable vulkan?
if (${ENABLE_BACKEND_VULKAN})
    target_compile_definitions(GRS.Test.Device.Generator PRIVATE -DENABLE_BACKEND_VULKAN)
endif()

# Enable dx12?
if (${ENABLE_BACKEND_DX12})
    target_compile_definitions(GRS.Test.Device.Generator PRIVATE -DENABLE_BACKEND_DX12)
endif()

# IDE source discovery
SetSourceDiscovery(GRS.Test.Device.Generator CXX Generator)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Test.Device.Generator PRIVATE /EHa)
endif()

# Includes
target_include_directories(GRS.Test.Device.Generator PUBLIC Generator/Include ${CMAKE_CURRENT_BINARY_DIR}/Generator/Include)

# Links
target_link_libraries(GRS.Test.Device.Generator PUBLIC GRS.Libraries.Common)

# Setup additional dependencies
ExternalProject_Link(GRS.Test.Device.Generator ArgParse)

# Test helper
function(Project_AddBackendTest OUT_GENERATED ARGS FEATURE_NAME TEST)
    # Get the source directory of the generator
    get_target_property(GeneratorSourceDir GRS.Libraries.Message.Generator SOURCE_DIR)

    # Schema
    if (NOT IS_ABSOLUTE ${TEST})
        set(Test ${CMAKE_CURRENT_SOURCE_DIR}/${TEST})
    else()
        set(Test ${TEST})
    endif()

    # Get the filename
    get_filename_component(TestName ${TEST} NAME_WE)
    set(TestOutPath Source/Test/${TestName}.cpp)

    # Compile the hlsl for all relevant backends
    set(HlslGenerated "")
    Project_AddHLSL(HlslGenerated cs_6_0 "${ARGS}" ${Test} Source/Test/${TestName} kShaderCode)

    # Get the source directory of the generator
    get_target_property(GeneratorSourceDir GRS.Test.Device.Generator SOURCE_DIR)

    # Template path
    set(Templates ${GeneratorSourceDir}/Generator/Templates)

    # Generate the test
    add_custom_command(
        OUTPUT ${TestOutPath}
        DEPENDS
            GRS.Test.Device.Generator
            ${Test}
            ${HlslGenerated}
            ${Templates}/MessageConstraint.cppt
            ${Templates}/Test.cppt
        COMMAND GRS.Test.Device.Generator
            -test ${Test}
            -feature ${FEATURE_NAME}
            -templates ${Templates}
            -shader ${TestName}
            -o ${TestOutPath}
    )

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${TestOutPath}" PARENT_SCOPE)
endfunction()

#----- Device -----#

add_library(
    GRS.Test.Device STATIC
    Source/main.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Test.Device CXX Include Source)

# Includes
target_include_directories(GRS.Test.Device PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# All tested backends
target_link_libraries(
    GRS.Test.Device PUBLIC

    # Libraries
    GRS.Libraries.Common
    GRS.Libraries.Backend
)

# Enable vulkan?
if (${ENABLE_BACKEND_VULKAN})
    target_link_libraries(GRS.Test.Device PUBLIC GRS.Backends.Vulkan.Layer GRS.Backends.Vulkan.TestDevice)
endif()

# Enable dx12?
if (${ENABLE_BACKEND_DX12})
    target_link_libraries(GRS.Test.Device PUBLIC GRS.Backends.DX12.Layer GRS.Backends.DX12.TestDevice)
endif()

# Setup additional dependencies
ExternalProject_Link(GRS.Test.Device Catch2)
ExternalProject_Link(GRS.Test.Device VMA)
ExternalProject_Link(GRS.Test.Device VulkanLoader vulkan-1)
