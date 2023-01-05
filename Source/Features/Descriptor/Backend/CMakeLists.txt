# Descriptor feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/Descriptor.xml Include/Schemas/Features)

# Create feature
add_library(
    Features.Descriptor.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(Features.Descriptor.Backend CXX Include Source Schemas)

# Includes
target_include_directories(Features.Descriptor.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Features.Descriptor.Backend PUBLIC Libraries.Common Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET Features.Descriptor.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Features.Descriptor.Backend> $<TARGET_FILE_DIR:Features.Descriptor.Backend>/Plugins/$<TARGET_FILE_NAME:Features.Descriptor.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest Descriptor)
Project_AddBackendTest(GeneratedTest "-Od" Descriptor Tests/Data/SimpleTest.hlsl)

Project_AddTest(
        NAME Features.Descriptor.Tests
        SOURCE ${GeneratedTest}
        LIBS Test.Device Features.Descriptor.Backend
)

#----- .Net bindings -----#

if (${ENABLE_UIX})
    Project_AddSchemaDotNet(Features.Descriptor.Backend.DotNet GeneratedCS)
endif()
