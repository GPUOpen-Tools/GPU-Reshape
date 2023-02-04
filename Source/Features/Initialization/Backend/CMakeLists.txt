# Initialization feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/Initialization.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.Initialization.Backend SHARED
    Source/Feature.cpp
    Source/SRBMaskingShaderProgram.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Initialization.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.Initialization.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(GRS.Features.Initialization.Backend PUBLIC GRS.Libraries.Common GRS.Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.Initialization.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.Initialization.Backend> $<TARGET_FILE_DIR:GRS.Features.Initialization.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.Initialization.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest Initialization)
Project_AddBackendTest(GeneratedTest "-Od" Initialization Tests/Data/SimpleTest.hlsl)

Project_AddTest(
    NAME GRS.Features.Initialization.Tests
    SOURCE ${GeneratedTest}
    LIBS GRS.Test.Device GRS.Features.Initialization.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Features.Initialization.Backend.DotNet GeneratedCS)
endif()
