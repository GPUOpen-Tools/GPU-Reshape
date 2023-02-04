# Descriptor feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/Descriptor.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.Descriptor.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Descriptor.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.Descriptor.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(GRS.Features.Descriptor.Backend PUBLIC GRS.Libraries.Common GRS.Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.Descriptor.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.Descriptor.Backend> $<TARGET_FILE_DIR:GRS.Features.Descriptor.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.Descriptor.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest Descriptor)
Project_AddBackendTest(GeneratedTest "-Od" Descriptor Tests/Data/SimpleTest.hlsl)

Project_AddTest(
        NAME GRS.Features.Descriptor.Tests
        SOURCE ${GeneratedTest}
        LIBS GRS.Test.Device GRS.Features.Descriptor.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Features.Descriptor.Backend.DotNet GeneratedCS)
endif()
