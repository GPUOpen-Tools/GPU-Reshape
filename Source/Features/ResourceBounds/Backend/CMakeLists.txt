# Resource bounds feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/ResourceBounds.xml Include/Schemas/Features)

# Create feature
add_library(
    Features.ResourceBounds.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(Features.ResourceBounds.Backend CXX Include Source Schemas)

# Includes
target_include_directories(Features.ResourceBounds.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Features.ResourceBounds.Backend PUBLIC Libraries.Common Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET Features.ResourceBounds.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Features.ResourceBounds.Backend> $<TARGET_FILE_DIR:Features.ResourceBounds.Backend>/Plugins/$<TARGET_FILE_NAME:Features.ResourceBounds.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest ResourceBounds)
Project_AddBackendTest(GeneratedTest "-Od" ResourceBounds Tests/Data/SimpleTest.hlsl)

# Create test app
add_executable(
    Features.ResourceBounds.Tests

    # Generated
    ${GeneratedTest}
)

# IDE source discovery
SetSourceDiscovery(Features.ResourceBounds.Tests CXX Tests)

# Links
target_link_libraries(Features.ResourceBounds.Tests PUBLIC Test.Device Features.ResourceBounds.Backend)

#----- .Net bindings -----#

if (${ENABLE_UIX})
    Project_AddSchemaDotNet(Features.ResourceBounds.Backend.DotNet ${GeneratedCS})
endif()
