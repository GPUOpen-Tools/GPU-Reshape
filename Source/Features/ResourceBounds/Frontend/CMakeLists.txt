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

#----- Hub -----#

# Create feature
add_library(
    GRS.Features.ResourceBounds.Frontend.Hub SHARED
    Hub/Source/Listener.cpp
    Hub/Source/Plugin.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.ResourceBounds.Frontend.Hub CXX Hub/Include Hub/Source)

# Includes
target_include_directories(GRS.Features.ResourceBounds.Frontend.Hub PUBLIC Hub/Include ${CMAKE_CURRENT_BINARY_DIR}/Hub/Include)

# Links
target_link_libraries(GRS.Features.ResourceBounds.Frontend.Hub PUBLIC GRS.Features.ResourceBounds.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET GRS.Features.ResourceBounds.Frontend.Hub POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GRS.Features.ResourceBounds.Frontend.Hub> $<TARGET_FILE_DIR:GRS.Features.ResourceBounds.Frontend.Hub>/Plugins/$<TARGET_FILE_NAME:GRS.Features.ResourceBounds.Frontend.Hub>
)

#----- .Net UIX -----#

if (${BUILD_UIX})
    # Add external projects
    include_external_msproject(GRS.Features.ResourceBounds.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(GRS.Features.ResourceBounds.Frontend.UIX PROPERTIES FOLDER Source/Features/ResourceBounds/Frontend)

    # Add runtime dependencies
    add_dependencies(
        GRS.Features.ResourceBounds.Frontend.UIX
        GRS.Features.ResourceBounds.Backend.DotNet
        UIX.Runtime
    )
endif()
