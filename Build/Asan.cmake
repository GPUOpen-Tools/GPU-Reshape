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


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

function(TargetEnableAsan TARGET)
    get_target_property(Imported ${TARGET} IMPORTED)
    if (${Imported})
        return()
    endif ()

    # TODO: Detect local environment
    target_link_directories(${TARGET} PRIVATE "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/lib/x64")
    target_link_libraries(${TARGET} PRIVATE clang_rt.asan_dynamic-x86_64 clang_rt.asan_dynamic_runtime_thunk-x86_64)
    target_link_options(${TARGET} PRIVATE /wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib)
endfunction()

function(add_library NAME)
    _add_library(${NAME} ${ARGN})
    TargetEnableAsan(${NAME})
endfunction(add_library)

function(add_executable NAME)
    _add_executable(${NAME} ${ARGN})
    TargetEnableAsan(${NAME})
endfunction(add_executable)
