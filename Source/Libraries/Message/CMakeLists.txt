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

#----- Generator lib -----#

# Create generator
add_library(
    GRS.Libraries.Message.Generator.Lib STATIC
    Generator/Source/Generators/PrimitiveTypeMap.cpp
    Generator/Source/Generators/MessageGenerator.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Message.Generator.Lib CXX Generator)

# Includes
target_include_directories(GRS.Libraries.Message.Generator.Lib PUBLIC Generator/Include ${CMAKE_CURRENT_BINARY_DIR}/Generator/Include)

# Local links
target_link_libraries(GRS.Libraries.Message.Generator.Lib PUBLIC GRS.Libraries.Common)

#----- Generator -----#

# Create generator
add_executable(
    GRS.Libraries.Message.Generator
    Generator/Source/Generator.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Message.Generator CXX Generator)

# Includes
target_include_directories(GRS.Libraries.Message.Generator PUBLIC Generator/Include ${CMAKE_CURRENT_BINARY_DIR}/Generator/Include)

# Local links
target_link_libraries(GRS.Libraries.Message.Generator PUBLIC GRS.Libraries.Common GRS.Libraries.Message.Generator.Lib)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Libraries.Message.Generator PRIVATE /EHs)
endif()

# Setup dependencies
ExternalProject_Link(GRS.Libraries.Message.Generator ArgParse)
ExternalProject_Link(GRS.Libraries.Message.Generator TinyXML2 tinyxml2)

# Schema helper
function(Project_AddSchemaEx OUT_GENERATED LIBS SCHEMA SCHEMA_OUT_PATH)
    # Get the source directory of the generator
    get_target_property(GeneratorSourceDir GRS.Libraries.Message.Generator SOURCE_DIR)

    if (NOT ${LIBS} STREQUAL "")
        set(LibPaths -libs ${LIBS})
    endif()

    # Schema
    set(Schema ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA})
    set(Templates ${GeneratorSourceDir}/Generator/Templates)

    # Set output path
    get_filename_component(SchemaName ${SCHEMA} NAME_WE)

    # Template dependencies
    set(TemplateDependencies "${Templates}/Message.cst" "${Templates}/Message.ht" "${Templates}/Schema.cst" "${Templates}/Schema.ht")

    # C++
    if (ON)
        set(OutputPath ${SCHEMA_OUT_PATH}/${SchemaName}.h)

        # Generate the schema
        add_custom_command(
            OUTPUT ${OutputPath}
            DEPENDS
                GRS.Libraries.Message.Generator
                ${LIBS}
                ${Schema}
                ${TemplateDependencies}
            COMMAND GRS.Libraries.Message.Generator
                -schemaxml ${Schema}
                -templates ${Templates}
                ${LibPaths}
                -lang cpp
                -opath ${SCHEMA_OUT_PATH}
        )

        # Set output
        list(APPEND ${OUT_GENERATED}CPP ${OutputPath})
        set(${OUT_GENERATED}CPP "${${OUT_GENERATED}CPP}" PARENT_SCOPE)
    endif()

    # C#
    if (${BUILD_UIX})
        set(OutputPath ${SCHEMA_OUT_PATH}/${SchemaName}.cs)

        # Generate the schema
        add_custom_command(
            OUTPUT ${OutputPath}.sham
            DEPENDS
                GRS.Libraries.Message.Generator
                ${LIBS}
                ${Schema}
                ${TemplateDependencies}
            COMMAND GRS.Libraries.Message.Generator
                -schemaxml ${Schema}
                -templates ${Templates}
                ${LibPaths}
                -lang cs
                -opath ${SCHEMA_OUT_PATH}
        )

        # Write dummy generated file, do not overwrite if it already exists to preserve the build cache
        if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${OutputPath})
            file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${OutputPath} "Generated file")
        endif()

        # Set output
        list(APPEND ${OUT_GENERATED}CS ${OutputPath})
        set(${OUT_GENERATED}CS "${${OUT_GENERATED}CS}" PARENT_SCOPE)

        # Set sham output
        list(APPEND ${OUT_GENERATED}CS_Sham ${OutputPath}.sham)
        set(${OUT_GENERATED}CS_Sham "${${OUT_GENERATED}CS_Sham}" PARENT_SCOPE)
    endif()
endfunction()

# Default schema helper
macro(Project_AddSchema OUT_GENERATED SCHEMA SCHEMA_OUT_PATH)
    Project_AddSchemaEx(${OUT_GENERATED} "" ${SCHEMA} ${SCHEMA_OUT_PATH})
endmacro()

#----- Message -----#

# Generate the schemas
Project_AddSchema(GeneratedSchema Schemas/Log.xml Include/Schemas)

add_library(
    GRS.Libraries.Message STATIC
    Source/Message.cpp
    Source/OrderedMessageStream.cpp

    # Generated
    ${GeneratedSchemaCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Message CXX Include Source)

# Links
target_link_libraries(GRS.Libraries.Message PUBLIC GRS.Libraries.Common)

# Includes
target_include_directories(GRS.Libraries.Message PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

#----- Tests -----#

# Schema
set(Schema ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Schemas/Schema.xml)
set(Template ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Message)

# Generate the schema
Project_AddSchema(Generated Tests/Schemas/Schema.xml Tests/Include/Schemas)

# Create test layer
add_executable(
    GRS.Libraries.Message.Tests
    Tests/Source/Main.cpp
    Tests/Source/Message.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Message.Tests CXX Tests)

# Include
target_include_directories(GRS.Libraries.Message.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(GRS.Libraries.Message.Tests Catch2)

# Links
target_link_libraries(GRS.Libraries.Message.Tests PUBLIC GRS.Libraries.Message)

#----- .Net bindings -----#

if (${BUILD_UIX})
    include(DotNet.cmake)
endif()
