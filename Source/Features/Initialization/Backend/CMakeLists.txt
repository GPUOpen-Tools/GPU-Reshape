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

# Initialization feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/Initialization.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.Initialization.Backend SHARED
    Source/Feature.cpp
    Source/MaskBlitShaderProgram.cpp
    Source/MaskCopyRangeShaderProgram.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Initialization.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.Initialization.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(
    GRS.Features.Initialization.Backend PUBLIC
    GRS.Libraries.Common
    GRS.Libraries.Backend
    GRS.Features.Descriptor.Backend
)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.Initialization.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.Initialization.Backend> $<TARGET_FILE_DIR:GRS.Features.Initialization.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.Initialization.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest Initialization)
Project_AddBackendTest(GeneratedTest "-Od" Initialization Tests/Data/InitializationSimpleTest.hlsl)
Project_AddBackendTest(GeneratedTest "-Od" Initialization Tests/Data/SampledTest.hlsl)

Project_AddTest(
    NAME GRS.Features.Initialization.Tests
    SOURCE ${GeneratedTest}
    LIBS GRS.Test.Device GRS.Features.Initialization.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Features.Initialization.Backend.DotNet GeneratedCS)
endif()
