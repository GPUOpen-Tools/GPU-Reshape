# Resource bounds feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/ResourceBounds.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.ResourceBounds.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.ResourceBounds.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.ResourceBounds.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(
    GRS.Features.ResourceBounds.Backend PUBLIC
    GRS.Libraries.Common
    GRS.Libraries.Backend
    GRS.Features.Descriptor.Backend
)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.ResourceBounds.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.ResourceBounds.Backend> $<TARGET_FILE_DIR:GRS.Features.ResourceBounds.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.ResourceBounds.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest ResourceBounds)
Project_AddBackendTest(GeneratedTest "-Od" ResourceBounds Tests/Data/ResourceBoundsSimpleTest.hlsl)

Project_AddTest(
        NAME GRS.Features.ResourceBounds.Tests
        SOURCE ${GeneratedTest}
        LIBS GRS.Test.Device GRS.Features.ResourceBounds.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Features.ResourceBounds.Backend.DotNet GeneratedCS)
endif()
