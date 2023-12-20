# 
# The MIT License (MIT)
# 
# Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
    target_compile_options(GRS.Test.Device.Generator PRIVATE /EHs)
endif()

# Includes
target_include_directories(GRS.Test.Device.Generator PUBLIC Generator/Include ${CMAKE_CURRENT_BINARY_DIR}/Generator/Include)

# Links
target_link_libraries(GRS.Test.Device.Generator PUBLIC GRS.Libraries.Common)

# Setup additional dependencies
ExternalProject_Link(GRS.Test.Device.Generator ArgParse)

# Test helper
function(Project_AddShaderType OUT_GENERATED ARGS HLSL)
    # Get the source directory of the generator
    get_target_property(GeneratorSourceDir GRS.Libraries.Message.Generator SOURCE_DIR)

    # Get the filename
    get_filename_component(HLSLName ${HLSL} NAME_WE)
    set(HLSLOutPath Source/HLSL/${HLSLName}.cpp)

    # Compile the hlsl for all relevant backends
    set(HlslGenerated "")
    Project_AddHLSL(HlslGenerated cs_6_0 "${ARGS}" ${HLSL} Source/HLSL/${HLSLName} kShaderCode ${ARGN})

    # Get the source directory of the generator
    get_target_property(GeneratorSourceDir GRS.Test.Device.Generator SOURCE_DIR)

    # Template path
    set(Templates ${GeneratorSourceDir}/Generator/Templates)

    # Generate the shader type
    add_custom_command(
        OUTPUT ${HLSLOutPath}
        DEPENDS
            GRS.Test.Device.Generator
            ${Test}
            ${HlslGenerated}
            ${Templates}/ShaderType.cppt
        COMMAND GRS.Test.Device.Generator
            -shaderType ${HLSLName}
            -name ${HLSLName}
            -templates ${Templates}
            -o ${HLSLOutPath}
    )

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${HLSLOutPath}" PARENT_SCOPE)
endfunction()

# Test helper
function(Project_AddBackendTest OUT_GENERATED ARGS FEATURE_NAME TEST)
    # Create shader
    Project_AddShaderType(${OUT_GENERATED} ${ARGS} ${TEST} ${ARGN})

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
    Source/ShaderHost.cpp
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
