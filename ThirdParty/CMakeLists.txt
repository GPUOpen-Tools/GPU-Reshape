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

include(ExternalProject)

# Disconnected third party fetch?
if (INSTALL_THIRD_PARTY)
    set(ThirdPartyDisconnected 0)
else()
    set(ThirdPartyDisconnected 1)
endif()

# Global
set(ThirdPartyDisconnected ${ThirdPartyDisconnected} CACHE INTERNAL "ThirdPartyDisconnected")

# Install all third party libraries
# TODO: While the pulled objects are generally trusted, there should be some safety mechanism
if (NOT THIN_X86_BUILD)
    include(Catch2.cmake)
    include(SPIRV.cmake)
    include(Vulkan.cmake)
    include(ArgParser.cmake)
    include(TinyXML2.cmake)
    include(VMA.cmake)
    include(Asio.cmake)
    include(Fmt.cmake)
    include(JSON.cmake)
    include(LibClang.cmake)
    include(GraphViz.cmake)
    include(DXC.cmake)
    include(DXIL.cmake)
    include(D3D12MA.cmake)
    include(UnorderedDense.cmake)
    include(BTree.cmake)
    include(ZLIB.cmake)

    if (${ENABLE_EXPERIMENTAL})
        include(Eigen.cmake)
        include(STB.cmake)
    endif()
endif()

# Shared between x86 and x64
include(Detour.cmake)
include(AMDAGS.cmake)
include(AgilitySDK.cmake)

# UI?
if (${BUILD_UIX})
    include(Avalonia.cmake)
endif ()

# Standard project installation
function(ExternalProject_Link Project Name)
    if (TARGET ${Name})
        add_dependencies(${Project} ${Name})
    endif()

    target_include_directories(${Project} SYSTEM PUBLIC ${CMAKE_BINARY_DIR}/External/include)
    target_link_directories(${Project} PUBLIC ${CMAKE_BINARY_DIR}/External/lib)

    if (NOT "${ARGN}" STREQUAL "")
        target_link_libraries(${Project} PUBLIC ${ARGN})
    endif()
endfunction()
