# 
# The MIT License (MIT)
# 
# Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

# Export stability feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/Waterfall.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.Waterfall.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Waterfall.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.Waterfall.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(GRS.Features.Waterfall.Backend PUBLIC GRS.Libraries.Common GRS.Libraries.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.Waterfall.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.Waterfall.Backend> $<TARGET_FILE_DIR:GRS.Features.Waterfall.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.Waterfall.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest Waterfall)
Project_AddBackendTest(GeneratedTest "-Od" Waterfall Tests/Data/WaterfallSimpleTest.hlsl)
Project_AddBackendTest(GeneratedTest "-O3" Waterfall Tests/Data/WaterfallSimplePhiTest.hlsl)
Project_AddBackendTest(GeneratedTest "-Od" Waterfall Tests/Data/WaterfallSimpleConstantTest.hlsl)
Project_AddBackendTest(GeneratedTest "-Od" Waterfall Tests/Data/WaterfallSimpleDivergenceTest.hlsl)
Project_AddBackendTest(GeneratedTest "-O3" Waterfall Tests/Data/WaterfallSimpleConditionalMaskingTest.hlsl)

Project_AddTest(
    NAME GRS.Features.Waterfall.Tests
    SOURCE ${GeneratedTest}
    LIBS GRS.Test.Device GRS.Features.Waterfall.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Features.Waterfall.Backend.DotNet GeneratedCS)
endif()
