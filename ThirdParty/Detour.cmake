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

include(FetchContent)

# Detour, used for API hooking
#   ! Must match detour version
if (NOT THIN_X86_BUILD)
    FetchContent_Declare(
        DetourFetch
        URL https://github.com/microsoft/Detours/zipball/4b8c659
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Detour
    )
    
    # Pull at compile time
    FetchContent_MakeAvailable(DetourFetch)
endif()

# Create layer
add_library(
    Detour STATIC
    Detour/src/creatwth.cpp
    Detour/src/detours.cpp
    Detour/src/disasm.cpp
    Detour/src/disolarm.cpp
    Detour/src/disolarm64.cpp
    Detour/src/disolia64.cpp
    Detour/src/disolx64.cpp
    Detour/src/disolx86.cpp
    Detour/src/image.cpp
    Detour/src/modules.cpp
)

# Get all includes
file(GLOB Includes Detour/src/*.h)

# Configure all includes
foreach (File ${Includes})
    get_filename_component(BaseName "${File}" NAME)
    configure_file(${File} ${CMAKE_BINARY_DIR}/External/include/Detour/${BaseName})
endforeach ()

# Set current version
#   ! Must match git version
target_compile_definitions(Detour PUBLIC -DDETOURS_VERSION=0x4c0c1)

# Compiler fixes
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Disable implicit pedantic mode set by standard
    get_target_property(DetourOptions Detour COMPILE_OPTIONS)
    list(REMOVE_ITEM DetourOptions "/std:c++20")
    set_property(TARGET Detour PROPERTY COMPILE_OPTIONS ${DetourOptions})
else()
    # Enable extensions
    target_compile_options(Detour PRIVATE -Wmicrosoft-goto)
endif()
