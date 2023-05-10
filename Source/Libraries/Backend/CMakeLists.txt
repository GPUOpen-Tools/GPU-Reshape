#----- Backend -----#

# Generate the schemas
Project_AddSchema(GeneratedLibSchema Schemas/Instrumentation.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/ShaderMetadata.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/PipelineMetadata.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/Config.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/SGUID.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/Diagnostic.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/Feature.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/Object.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/Versioning.xml Include/Schemas)

add_library(
    GRS.Libraries.Backend STATIC
    Source/Backend.cpp
    Source/FeatureHost.cpp
    Source/Environment.cpp
    Source/StartupEnvironment.cpp
    Source/ShaderSGUIDHostListener.cpp
    Source/IL/PrettyPrint.cpp
    Source/IL/Function.cpp
    Source/IL/BasicBlock.cpp

    # Generated schemas
    ${GeneratedLibSchemaCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Libraries.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(GRS.Libraries.Backend PUBLIC GRS.Libraries.Message GRS.Libraries.Bridge GRS.Services.HostResolver)

#----- Generators -----#

add_library(
    GRS.Libraries.Backend.Generator SHARED
    Generator/Source/ShaderExportGenerator.cpp
    Generator/Source/Plugin.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Backend.Generator CXX Generator)

# Includes
target_include_directories(GRS.Libraries.Backend.Generator PUBLIC Generator/Include ${CMAKE_CURRENT_BINARY_DIR}/Generator/Include)

# Links
target_link_libraries(GRS.Libraries.Backend.Generator PUBLIC GRS.Libraries.Backend GRS.Libraries.Message.Generator.Lib)

# Default schema helper
macro(Project_AddShaderSchema OUT_GENERATED SCHEMA SCHEMA_OUT_PATH)
    Project_AddSchemaEx(${OUT_GENERATED} GRS.Libraries.Backend.Generator ${SCHEMA} ${SCHEMA_OUT_PATH})
endmacro()

#----- Tests -----#

# Generate the schema
Project_AddSchema(GeneratedTestSchema Tests/Schemas/Feature.xml Tests/Include/Schemas)

# Create test layer
add_executable(
    GRS.Libraries.Backend.Tests
    Tests/Source/Main.cpp
    Tests/Source/Emitter.cpp
    Tests/Source/Feature.cpp
    Tests/Source/BasicBlock.cpp

    # Generated
    ${GeneratedTestSchemaCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Backend.Tests CXX Tests)

# Includes
target_include_directories(GRS.Libraries.Backend.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(GRS.Libraries.Backend.Tests Catch2)

# Links
target_link_libraries(GRS.Libraries.Backend.Tests PUBLIC GRS.Libraries.Backend)

#---- .Net Bindings ----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Libraries.Backend.Schema.DotNet GeneratedLibSchemaCS)
endif()
