# 
# The MIT License (MIT)
# 
# Copyright (c) 2024 Advanced Micro Devices, Inc.,
# Fatalist Development AB (Avalanche Studio Group),
# and Miguel Petersen.
# 
# All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy 
# of this software and associated documentation files (the "Software"), to deal 
# in the Software without restriction, including without limitation the rights 
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
# of the Software, and to permit persons to whom the Software is furnished to do so, 
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all 
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 

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
Project_AddSchema(GeneratedLibSchema Schemas/PDB.xml Include/Schemas)

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

# Setup dependencies
ExternalProject_Link(GRS.Libraries.Backend JSON)

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
