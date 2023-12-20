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

#----- Automation -----#

add_library(
    GRS.Test.Automation STATIC
    Source/Connection/Connection.cpp
    Source/Connection/PooledMessage.cpp
    Source/Connection/PoolingListener.cpp
    Source/Pass/ApplicationPass.cpp
    Source/Pass/ClickPass.cpp
    Source/Pass/KeyPass.cpp
    Source/Pass/ScreenshotPass.cpp
    Source/Pass/SequencePass.cpp
    Source/Pass/StandardInstrumentationPass.cpp
    Source/Pass/WaitForScreenshotPass.cpp
    Source/Pass/WaitPass.cpp
    Source/Data/ApplicationData.cpp
    Source/Win32/Window.cpp
    Source/TestContainer.cpp
    Source/Parser.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Test.Automation CXX Include Source)

# Includes
target_include_directories(GRS.Test.Automation PUBLIC Include)

# Dependencies
target_link_libraries(
    GRS.Test.Automation PUBLIC
    GRS.Libraries.Transform
    GRS.Services.Discovery
    GRS.Services.HostResolver
    Dwmapi
)

# Setup additional dependencies
ExternalProject_Link(GRS.Test.Automation JSON)

#----- Runner -----#

add_executable(
    GRS.Test.Runner
    Runner/main.cpp
)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Test.Runner PRIVATE /EHs)
endif()

# IDE source discovery
SetSourceDiscovery(GRS.Test.Runner CXX Runner)

# Includes
target_include_directories(GRS.Test.Runner PUBLIC Include)

# Links
target_link_libraries(GRS.Test.Runner PUBLIC GRS.Test.Automation)

# Setup additional dependencies
ExternalProject_Link(GRS.Test.Runner ArgParse)
