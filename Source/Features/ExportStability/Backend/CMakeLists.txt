# Export stability feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/ExportStability.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.ExportStability.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.ExportStability.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.ExportStability.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(GRS.Features.ExportStability.Backend PUBLIC GRS.Libraries.Common GRS.Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.ExportStability.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.ExportStability.Backend> $<TARGET_FILE_DIR:GRS.Features.ExportStability.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.ExportStability.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest ExportStability)
Project_AddBackendTest(GeneratedTest "-Od" ExportStability Tests/Data/SimpleTest.hlsl)

Project_AddTest(
        NAME GRS.Features.ExportStability.Tests
        SOURCE ${GeneratedTest}
        LIBS GRS.Test.Device GRS.Features.ExportStability.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Features.ExportStability.Backend.DotNet GeneratedCS)
endif()
