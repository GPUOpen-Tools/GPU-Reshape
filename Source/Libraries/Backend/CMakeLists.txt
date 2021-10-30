
#----- Backend -----#

add_library(
    Libraries.Backend STATIC
    Source/Backend.cpp
)

# Includes
target_include_directories(Libraries.Backend PUBLIC Include)

# Links
target_link_libraries(Libraries.Backend PUBLIC Libraries.Message)

#----- Tests -----#

# Generate the schema
Project_AddSchema(Generated Tests/Schemas/Feature.xml Tests/Include/Schemas)

# Create test layer
add_executable(
    Libraries.Backend.Tests
    Tests/Source/Main.cpp
    Tests/Source/Emitter.cpp
    Tests/Source/Feature.cpp

    # Generated
    ${Generated}
)

# Includes
target_include_directories(Libraries.Backend.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(Libraries.Backend.Tests Catch2)

# Links
target_link_libraries(Libraries.Backend.Tests Libraries.Backend)
