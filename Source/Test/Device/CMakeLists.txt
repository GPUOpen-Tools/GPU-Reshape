#----- Generator -----#

add_executable(
    Test.Device.Generator
    Generator/Source/Parser.cpp
    Generator/Source/Assembler.cpp
    Generator/Source/main.cpp
)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(Test.Device.Generator PRIVATE /EHa)
endif()

# Includes
target_include_directories(Test.Device.Generator PUBLIC Generator/Include ${CMAKE_CURRENT_BINARY_DIR}/Generator/Include)

# Links
target_link_libraries(Test.Device.Generator PUBLIC Libraries.Common)

# Setup additional dependencies
ExternalProject_Link(Test.Device.Generator ArgParse)

# Test helper
function(Project_AddBackendTest OUT_GENERATED FEATURE_NAME TEST)
    # Get the source directory of the generator
    get_target_property(GeneratorSourceDir Libraries.Message.Generator SOURCE_DIR)

    # Schema
    set(Test ${CMAKE_CURRENT_SOURCE_DIR}/${TEST})

    # Get the filename
    get_filename_component(TestName ${TEST} NAME_WE)
    set(TestOutPath Source/Test/${TestName}.cpp)

    # Compile the hlsl for all relevant backends
    set(HlslGenerated "")
    Project_AddHLSL(HlslGenerated cs_6_0 ${TEST} Source/Test/${TestName}.hlsl.h kShaderCode)

    # Get the source directory of the generator
    get_target_property(GeneratorSourceDir Test.Device.Generator SOURCE_DIR)

    # Template path
    set(Templates ${GeneratorSourceDir}/Generator/Templates)

    # Generate the test
    add_custom_command(
        OUTPUT ${TestOutPath}
        DEPENDS
            Test.Device.Generator
            ${TEST}
            ${HlslGenerated}
            ${Templates}/MessageConstraint.cppt
            ${Templates}/Test.cppt
        COMMAND Test.Device.Generator
            -test ${Test}
            -feature ${FEATURE_NAME}
            -templates ${Templates}
            -shader ${TestName}.hlsl.h
            -o ${TestOutPath}
    )

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${TestOutPath}" PARENT_SCOPE)
endfunction()

#----- Device -----#

add_library(
    Test.Device STATIC
    Source/main.cpp
)

# Includes
target_include_directories(Test.Device PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# All tested backends
target_link_libraries(
    Test.Device PUBLIC

    # Vulkan
    Backends.Vulkan.Layer Backends.Vulkan.TestDevice

    # Libraries
    Libraries.Common
    Libraries.Backend
)

# Setup additional dependencies
ExternalProject_Link(Test.Device Catch2)
ExternalProject_Link(Test.Device VMA)
ExternalProject_Link(Test.Device VulkanLoader vulkan-1)