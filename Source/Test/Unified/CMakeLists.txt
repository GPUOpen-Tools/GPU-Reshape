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


#----- Unified tests -----#

add_executable(
    GRS.Test.Unified
    Source/main.cpp
)

function(Project_AddUnifiedTestLibrary NAME)
    target_link_libraries(GRS.Test.Unified PRIVATE ${NAME})
endfunction()

function(Project_AddTest)
    cmake_parse_arguments(
        ARGS
        "" # Options
        "NAME" # One Value
        "SOURCE;LIBS" # Multi Value
        ${ARGN}
    )

    # Get properties
    get_target_property(SourceDir GRS.Test.Unified SOURCE_DIR)

    # Create test library
    add_library(
        ${ARGS_NAME}.Lib OBJECT
        ${ARGS_SOURCE}
    )

    if(NOT "${ARGS_LIBS}" STREQUAL "")
        target_link_libraries(${ARGS_NAME}.Lib PUBLIC ${ARGS_LIBS})
    endif()

    # Create test app
    add_executable(
        ${ARGS_NAME}
        ${SourceDir}/Source/main.cpp
    )

    target_link_libraries(${ARGS_NAME} PUBLIC ${ARGS_NAME}.Lib)

    Project_AddUnifiedTestLibrary(${ARGS_NAME}.Lib)

    # IDE source discovery
    SetSourceDiscovery(${ARGS_NAME}.Lib CXX Tests)
    SetSourceDiscovery(${ARGS_NAME} CXX Tests)
endfunction()
