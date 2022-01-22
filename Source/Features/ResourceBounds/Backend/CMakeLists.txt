# Resource bounds feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/ResourceBounds.xml Include/Schemas/Features)

# Create feature
add_library(
    Features.ResourceBounds.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${Generated}
)

# Includes
target_include_directories(Features.ResourceBounds.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Features.ResourceBounds.Backend PUBLIC Libraries.Common Libraries.Backend)

# Set all outputs to Bin/Config/Plugins
set_target_properties(
    Features.ResourceBounds.Backend PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/Plugins
)


#----- Test Application -----#

# Test files
set(Generated "")
Project_AddBackendTest(Generated ResourceBounds Tests/Data/SimpleTest.hlsl)

# Create test app
add_executable(
    Features.ResourceBounds.Tests
    Tests/Source/main.cpp

    # Generated
    ${Generated}
)

# Links
target_link_libraries(Features.ResourceBounds.Tests PUBLIC Test.Device Features.ResourceBounds.Backend)
