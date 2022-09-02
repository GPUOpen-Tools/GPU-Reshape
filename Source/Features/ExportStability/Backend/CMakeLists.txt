# Export stability feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/ExportStability.xml Include/Schemas/Features)

# Create feature
add_library(
    Features.ExportStability.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(Features.ExportStability.Backend CXX Include Source Schemas)

# Includes
target_include_directories(Features.ExportStability.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Features.ExportStability.Backend PUBLIC Libraries.Common Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET Features.ExportStability.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Features.ExportStability.Backend> $<TARGET_FILE_DIR:Features.ExportStability.Backend>/Plugins/$<TARGET_FILE_NAME:Features.ExportStability.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest ExportStability)
Project_AddBackendTest(GeneratedTest "-Od" ExportStability Tests/Data/SimpleTest.hlsl)

# Create test app
add_executable(
    Features.ExportStability.Tests

    # Generated
    ${GeneratedTest}
)

# IDE source discovery
SetSourceDiscovery(Features.ExportStability.Tests CXX Tests)

# Links
target_link_libraries(Features.ExportStability.Tests PUBLIC Test.Device Features.ExportStability.Backend)

#----- .Net bindings -----#

if (${ENABLE_UIX})
    Project_AddSchemaDotNet(Features.ExportStability.Backend.DotNet ${GeneratedCS})
endif()
