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


#------ Standalone ------#

# Create standalone
add_executable(
    GRS.Services.HostResolver.Standalone
    Standalone/main.cpp
)

# Build convenience, kill the process
if (WIN32)
    add_custom_command(
        TARGET GRS.Services.HostResolver.Standalone
        PRE_BUILD COMMAND ${CMAKE_SOURCE_DIR}/Build/Utils/KillHostResolverService.bat
    )
endif()

# IDE source discovery
SetSourceDiscovery(GRS.Services.HostResolver.Standalone CXX Standalone)

# Include directories
target_include_directories(GRS.Services.HostResolver.Standalone PUBLIC Include)

# Dependencies
target_link_libraries(GRS.Services.HostResolver.Standalone PUBLIC GRS.Libraries.Bridge)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Services.HostResolver.Standalone PRIVATE /EHa)
endif()

#------ Service ------#

# Create library
add_library(
    GRS.Services.HostResolver STATIC
    Source/HostResolverService.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Services.HostResolver CXX Include Source)

# Include directories
target_include_directories(GRS.Services.HostResolver PUBLIC Include)

# Dependencies
target_link_libraries(GRS.Services.HostResolver PUBLIC GRS.Libraries.Bridge)

# Ensure the standalone is compiled
add_dependencies(GRS.Services.HostResolver GRS.Services.HostResolver.Standalone)

#----- .Net bindings -----#

if (${BUILD_UIX})
    include(DotNet.cmake)
endif()
