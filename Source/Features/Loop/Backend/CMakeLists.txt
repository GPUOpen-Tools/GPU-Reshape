# Loop feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/Loop.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.Loop.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Loop.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.Loop.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(
    GRS.Features.Loop.Backend PUBLIC
    GRS.Libraries.Common
    GRS.Libraries.Backend
    GRS.Features.Descriptor.Backend
)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.Loop.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.Loop.Backend> $<TARGET_FILE_DIR:GRS.Features.Loop.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.Loop.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest Loop)
Project_AddBackendTest(GeneratedTest "-Od" Loop Tests/Data/LoopSimpleTest.hlsl)

Project_AddTest(
        NAME GRS.Features.Loop.Tests
        SOURCE ${GeneratedTest}
        LIBS GRS.Test.Device GRS.Features.Loop.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Features.Loop.Backend.DotNet GeneratedCS)
endif()
