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

# Expected SHA256
set(AgilitySDKSHA256 "2df788c3a020cca225a489062a4440733e8b2e1951ccbaa487af2e60da33635b")

# Intermediate path
set(Path ${CMAKE_BINARY_DIR}/Nuget/AgilitySDK.zip)

# Already present?
if (EXISTS ${Path})
    file(SHA256 ${Path} Checksum)
else()
    set(Checksum 0)
endif()

# Current version OK?
if (NOT "${Checksum}" STREQUAL ${AgilitySDKSHA256})
    # Download SDK
    file(DOWNLOAD https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.711.3-preview ${Path})

    # Validate SHA256
    file(SHA256 ${Path} Checksum)
    if (NOT "${Checksum}" STREQUAL ${AgilitySDKSHA256})
        message(FATAL_ERROR "AgilitySDK checksum mismatch\n\tGot: ${Checksum}\n\tExpected: ${AgilitySDKSHA256}")
    endif()
    
    # Ensure path exists
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AgilitySDK/)

    # Unzip
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${Path}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AgilitySDK/
    )
    
    # Get all includes
    file(GLOB Sources AgilitySDK/build/native/include/*)
    
    # Configure all includes
    foreach (File ${Sources})
        if (IS_DIRECTORY ${File})
            continue()
        endif()
    
        # Copy file
        get_filename_component(BaseName "${File}" NAME)
        configure_file(${File} ${CMAKE_BINARY_DIR}/External/include/AgilitySDK/${BaseName})
    endforeach ()
endif()
