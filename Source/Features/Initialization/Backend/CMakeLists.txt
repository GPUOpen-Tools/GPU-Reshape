# Initialization feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/Initialization.xml Include/Schemas/Features)

# Create feature
add_library(
    Features.Initialization.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(Features.Initialization.Backend CXX Include Source Schemas)

# Includes
target_include_directories(Features.Initialization.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Features.Initialization.Backend PUBLIC Libraries.Common Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET Features.Initialization.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Features.Initialization.Backend> $<TARGET_FILE_DIR:Features.Initialization.Backend>/Plugins/$<TARGET_FILE_NAME:Features.Initialization.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest Initialization)
Project_AddBackendTest(GeneratedTest "-Od" Initialization Tests/Data/SimpleTest.hlsl)

# Create test app
add_executable(
    Features.Initialization.Tests

    # Generated
    ${GeneratedTest}
)

# IDE source discovery
SetSourceDiscovery(Features.Initialization.Tests CXX Tests)

# Links
target_link_libraries(Features.Initialization.Tests PUBLIC Test.Device Features.Initialization.Backend)

#----- .Net bindings -----#

if (${ENABLE_UIX})
    Project_AddSchemaDotNet(Features.Initialization.Backend.DotNet GeneratedCS)
endif()
