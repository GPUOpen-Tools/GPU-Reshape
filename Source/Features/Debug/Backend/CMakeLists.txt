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

# Debug feature library

# Generate the schema
Project_AddSchema(Generated Schemas/DebugConfig.xml Include/Schemas/Features)
Project_AddShaderSchema(Generated Schemas/Debug.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.Debug.Backend SHARED
    Source/Feature.cpp
    Source/LockShaderProgram.cpp
    Source/LooseAcquisitionProgram.cpp
    Source/ResetHeaderProgram.cpp
    Source/SetupPredicateProgram.cpp
    Source/SetupIndirectCopyProgram.cpp
    Source/IndirectCopyProgram.cpp
    Source/SetupPredicateProgram.cpp
    Source/FinalizePredicateProgram.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Debug.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.Debug.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(
    GRS.Features.Debug.Backend PUBLIC
    GRS.Libraries.Common
    GRS.Libraries.Backend
    GRS.Libraries.Addressing
    GRS.Features.Descriptor.Backend
)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.Debug.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.Debug.Backend> $<TARGET_FILE_DIR:GRS.Features.Debug.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.Debug.Backend>
)

#----- Test Application -----#

if (${ENABLE_UNIT_TESTS})
    # Test files
    Project_AddBackendTest_Common(GeneratedTest Debug)
    
    Project_AddTest(
            NAME GRS.Features.Debug.Tests
            SOURCE ${GeneratedTest}
            LIBS GRS.Test.Device GRS.Features.Debug.Backend
    )
endif()

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddDotNetGeneratedMerge(GRS.Features.Debug.Backend.DotNet.GenMerge GeneratedCS)
    add_subdirectory(DotNet)
endif()
