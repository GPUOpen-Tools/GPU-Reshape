#----- Generator lib -----#

# Create generator
add_library(
    Libraries.Message.Generator.Lib STATIC
    Generator/Source/Generators/PrimitiveTypeMap.cpp
    Generator/Source/Generators/MessageGenerator.cpp
)

# IDE source discovery
SetSourceDiscovery(Libraries.Message.Generator.Lib CXX Generator)

# Includes
target_include_directories(Libraries.Message.Generator.Lib PUBLIC Generator/Include ${CMAKE_CURRENT_BINARY_DIR}/Generator/Include)

# Local links
target_link_libraries(Libraries.Message.Generator.Lib PUBLIC Libraries.Common)

#----- Generator -----#

# Create generator
add_executable(
    Libraries.Message.Generator
    Generator/Source/Generator.cpp
)

# IDE source discovery
SetSourceDiscovery(Libraries.Message.Generator CXX Generator)

# Includes
target_include_directories(Libraries.Message.Generator PUBLIC Generator/Include ${CMAKE_CURRENT_BINARY_DIR}/Generator/Include)

# Local links
target_link_libraries(Libraries.Message.Generator PUBLIC Libraries.Common Libraries.Message.Generator.Lib)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(Libraries.Message.Generator PRIVATE /EHa)
endif()

# Setup dependencies
ExternalProject_Link(Libraries.Message.Generator ArgParse)
ExternalProject_Link(Libraries.Message.Generator TinyXML2 tinyxml2)

# Schema helper
function(Project_AddSchemaEx OUT_GENERATED LIBS SCHEMA SCHEMA_OUT_PATH)
    # Get the source directory of the generator
    get_target_property(GeneratorSourceDir Libraries.Message.Generator SOURCE_DIR)

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
                Libraries.Message.Generator
                ${LIBS}
                ${Schema}
                ${TemplateDependencies}
            COMMAND Libraries.Message.Generator
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
                Libraries.Message.Generator
                ${LIBS}
                ${Schema}
                ${TemplateDependencies}
            COMMAND Libraries.Message.Generator
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
    Libraries.Message STATIC
    Source/Message.cpp
    Source/OrderedMessageStream.cpp

    # Generated
    ${GeneratedSchemaCPP}
)

# IDE source discovery
SetSourceDiscovery(Libraries.Message CXX Include Source)

# Links
target_link_libraries(Libraries.Message PUBLIC Libraries.Common)

# Includes
target_include_directories(Libraries.Message PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

#----- Tests -----#

# Schema
set(Schema ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Schemas/Schema.xml)
set(Template ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Message)

# Generate the schema
Project_AddSchema(Generated Tests/Schemas/Schema.xml Tests/Include/Schemas)

# Create test layer
add_executable(
    Libraries.Message.Tests
    Tests/Source/Main.cpp
    Tests/Source/Message.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(Libraries.Message.Tests CXX Tests)

# Include
target_include_directories(Libraries.Message.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(Libraries.Message.Tests Catch2)

# Links
target_link_libraries(Libraries.Message.Tests PUBLIC Libraries.Message)

#----- .Net bindings -----#

if (${BUILD_UIX})
    include(DotNet.cmake)
endif()
