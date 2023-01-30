# Concurrency feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/Concurrency.xml Include/Schemas/Features)

# Create feature
add_library(
    Features.Concurrency.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(Features.Concurrency.Backend CXX Include Source Schemas)

# Includes
target_include_directories(Features.Concurrency.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Features.Concurrency.Backend PUBLIC Libraries.Common Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET Features.Concurrency.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Features.Concurrency.Backend> $<TARGET_FILE_DIR:Features.Concurrency.Backend>/Plugins/$<TARGET_FILE_NAME:Features.Concurrency.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest Concurrency)
Project_AddBackendTest(GeneratedTest "-Od" Concurrency Tests/Data/SimpleTest.hlsl)

Project_AddTest(
        NAME Features.Concurrency.Tests
        SOURCE ${GeneratedTest}
        LIBS Test.Device Features.Concurrency.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(Features.Concurrency.Backend.DotNet GeneratedCS)
endif()
