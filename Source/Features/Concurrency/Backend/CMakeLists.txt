# Concurrency feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/Concurrency.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.Concurrency.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Concurrency.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.Concurrency.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(GRS.Features.Concurrency.Backend PUBLIC GRS.Libraries.Common GRS.Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.Concurrency.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.Concurrency.Backend> $<TARGET_FILE_DIR:GRS.Features.Concurrency.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.Concurrency.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest Concurrency)
Project_AddBackendTest(GeneratedTest "-Od" Concurrency Tests/Data/SimpleTest.hlsl)

Project_AddTest(
        NAME GRS.Features.Concurrency.Tests
        SOURCE ${GeneratedTest}
        LIBS GRS.Test.Device GRS.Features.Concurrency.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Features.Concurrency.Backend.DotNet GeneratedCS)
endif()
