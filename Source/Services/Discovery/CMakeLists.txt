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


# Create library
add_library(
    GRS.Services.Discovery STATIC
    Source/DiscoveryService.cpp
    Source/NotifyIconDiscoveryListener.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Services.Discovery CXX Include Source)

# Include directories
target_include_directories(GRS.Services.Discovery PUBLIC Include)

# Dependencies
target_link_libraries(GRS.Services.Discovery PUBLIC GRS.Libraries.Discovery GRS.Libraries.Bridge GRS.Libraries.Backend)

# External
ExternalProject_Link(GRS.Services.Discovery Detour Detour)

#------ Standalone ------#

# Create standalone
add_executable(
    GRS.Services.Discovery.Standalone
    Standalone/main.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Services.Discovery.Standalone CXX Standalone)

# Dependencies
target_link_libraries(GRS.Services.Discovery.Standalone PUBLIC GRS.Services.Discovery)

# Create dependencies on existing listeners, for convenience
if (${ENABLE_BACKEND_VULKAN})
    add_dependencies(GRS.Services.Discovery.Standalone GRS.Backends.Vulkan.Discovery)
endif()
if (${ENABLE_BACKEND_DX12})
    add_dependencies(GRS.Services.Discovery.Standalone GRS.Backends.DX12.Discovery)
endif()

# Pull in all features, for convenience
add_dependencies(GRS.Services.Discovery.Standalone Features)

#------ Cleanup tool ------#

# Create cleanup tool
add_executable(
    GRS.Services.Discovery.Cleanup
    Cleanup/main.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Services.Discovery.Cleanup CXX Cleanup)

# Dependencies
target_link_libraries(GRS.Services.Discovery.Cleanup PUBLIC GRS.Services.Discovery)

# Create dependencies on existing listeners, for convenience
if (${ENABLE_BACKEND_VULKAN})
    add_dependencies(GRS.Services.Discovery.Cleanup GRS.Backends.Vulkan.Discovery)
endif()
if (${ENABLE_BACKEND_DX12})
    add_dependencies(GRS.Services.Discovery.Cleanup GRS.Backends.DX12.Discovery)
endif()

#----- .Net bindings -----#

if (${BUILD_UIX})
    include(DotNet.cmake)
endif()
