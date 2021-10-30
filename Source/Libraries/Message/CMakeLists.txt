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

#----- Message -----#

add_library(
    Libraries.Message STATIC
    Source/Message.cpp
)

# Includes
target_include_directories(Libraries.Message PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

#----- Tests -----#

# Schema
set(Schema ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Schemas/Schema.xml)
set(Template ${CMAKE_CURRENT_SOURCE_DIR}/Generator/Templates/Message)

# Generate the schema
add_custom_command(
    OUTPUT Tests/Include/Schemas/Schema.h
    DEPENDS
        Libraries.Message.Generator
        ${Schema}
    COMMAND Libraries.Message.Generator
        -schemaxml ${Schema}
        -template ${Template}
        -lang cpp
        -opath Tests/Include/Schemas
)

# Create test layer
add_executable(
    Libraries.Message.Tests
    Tests/Source/Main.cpp
    Tests/Source/Message.cpp

    # Generated
    Tests/Include/Schemas/Schema.h
)

# Include
target_include_directories(Libraries.Message.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(Libraries.Message.Tests Catch2)

# Links
target_link_libraries(Libraries.Message.Tests Libraries.Message)
