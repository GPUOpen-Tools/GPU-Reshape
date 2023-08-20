# 
# The MIT License (MIT)
# 
# Copyright (c) 2023 Miguel Petersen
# Copyright (c) 2023 Advanced Micro Devices, Inc
# Copyright (c) 2023 Avalanche Studios Group
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

# x86?
if (THIN_X86_BUILD)
    # Create the RelFunTBL target, provides x86 specific relative function tables for hooking
    add_executable(GRS.Backends.DX12.Service.RelFunTBL Service/Source/RelFunTBL.cpp)
    
    # Include directories
    target_include_directories(GRS.Backends.DX12.Service.RelFunTBL PUBLIC Service/Include)
    
    # Dependencies
    target_link_libraries(GRS.Backends.DX12.Service.RelFunTBL PUBLIC GRS.Libraries.Common)
endif()

# Create service
add_library(
    GRS.Backends.DX12.Bootstrapper${ARCH_POSTFIX} SHARED
    Bootstrapper/Source/DLL.cpp
    Bootstrapper.def
)

# IDE source discovery
if (NOT THIN_X86_BUILD)
    SetSourceDiscovery(GRS.Backends.DX12.Bootstrapper${ARCH_POSTFIX} CXX Bootstrapper)
else()
    target_compile_definitions(GRS.Backends.DX12.Bootstrapper${ARCH_POSTFIX} PRIVATE THIN_X86=1)
endif()

# Include directories
target_include_directories(GRS.Backends.DX12.Bootstrapper${ARCH_POSTFIX} PUBLIC Layer/Include Bootstrapper/Include)

# Dependencies
target_link_libraries(GRS.Backends.DX12.Bootstrapper${ARCH_POSTFIX} PUBLIC GRS.Libraries.Common)

# External dependencies
ExternalProject_Link(GRS.Backends.DX12.Bootstrapper${ARCH_POSTFIX} Detour Detour)
