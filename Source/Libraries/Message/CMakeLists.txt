#----- Generator -----#

# Create generator
add_executable(
    Libraries.Message.Generator
    Generator/Source/Generator.cpp
    Generator/Source/CPP.cpp
    Generator/Source/CS.cpp
)

# Local links
target_link_libraries(Libraries.Message.Generator Libraries.Common)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(Libraries.Message.Generator PRIVATE /EHa)
endif()

# Setup dependencies
ExternalProject_Link(Libraries.Message.Generator ArgParse)
ExternalProject_Link(Libraries.Message.Generator TinyXML2 tinyxml2)

# Schema helper
function(Project_AddSchema OUT_GENERATED SCHEMA SCHEMA_OUT_PATH)
    # Get the source directory of the generator
    get_target_property(GeneratorSourceDir Libraries.Message.Generator SOURCE_DIR)

    # Schema
    set(Schema ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA})
    set(Template ${GeneratorSourceDir}/Generator/Templates/Message)

    # Set output path
    get_filename_component(SchemaName ${SCHEMA} NAME_WE)
    set(OutputPath ${SCHEMA_OUT_PATH}/${SchemaName}.h)

    # Generate the schema
    add_custom_command(
        OUTPUT ${OutputPath}
        DEPENDS
            Libraries.Message.Generator
            ${Schema}
        COMMAND Libraries.Message.Generator
            -schemaxml ${Schema}
            -template ${Template}
            -lang cpp
            -opath ${SCHEMA_OUT_PATH}
    )

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${OutputPath}" PARENT_SCOPE)
endfunction()

#----- Message -----#

add_library(
    Libraries.Message STATIC
    Source/Message.cpp
    Source/OrderedMessageStream.cpp
)

# Links
target_link_libraries(Libraries.Message Libraries.Common)

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
    ${Generated}
)

# Include
target_include_directories(Libraries.Message.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(Libraries.Message.Tests Catch2)

# Links
target_link_libraries(Libraries.Message.Tests Libraries.Message)
