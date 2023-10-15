# 
# The MIT License (MIT)
# 
# Copyright (c) 2023 Miguel Petersen
# Copyright (c) 2023 Advanced Micro Devices, Inc
# Copyright (c) 2023 Fatalist Development AB
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

# Resource bounds feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/ResourceBounds.xml Include/Schemas/Features)

# Create feature
add_library(
    GRS.Features.ResourceBounds.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${GeneratedCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.ResourceBounds.Backend CXX Include Source Schemas)

# Includes
target_include_directories(GRS.Features.ResourceBounds.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(
    GRS.Features.ResourceBounds.Backend PUBLIC
    GRS.Libraries.Common
    GRS.Libraries.Backend
    GRS.Features.Descriptor.Backend
)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.ResourceBounds.Backend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.ResourceBounds.Backend> $<TARGET_FILE_DIR:GRS.Features.ResourceBounds.Backend>/Plugins/$<TARGET_FILE_NAME:GRS.Features.ResourceBounds.Backend>
)

#----- Test Application -----#

# Test files
Project_AddBackendTest_Common(GeneratedTest ResourceBounds)
Project_AddBackendTest(GeneratedTest "-Od" ResourceBounds Tests/Data/ResourceBoundsSimpleTest.hlsl)

Project_AddTest(
        NAME GRS.Features.ResourceBounds.Tests
        SOURCE ${GeneratedTest}
        LIBS GRS.Test.Device GRS.Features.ResourceBounds.Backend
)

#----- .Net bindings -----#

if (${BUILD_UIX})
    Project_AddSchemaDotNet(GRS.Features.ResourceBounds.Backend.DotNet GeneratedCS)
endif()
