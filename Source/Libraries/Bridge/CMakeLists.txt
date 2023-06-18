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


#----- Bridge -----#

# Generate the schemas
Project_AddSchema(GeneratedLibSchema Schemas/PingPong.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/HostResolve.xml Include/Schemas)

add_library(
    GRS.Libraries.Bridge STATIC
    Source/MemoryBridge.cpp
    Source/HostServerBridge.cpp
    Source/RemoteClientBridge.cpp
    Source/Network/PingPongListener.cpp
    Source/Log/LogConsoleListener.cpp
    Source/Log/LogBuffer.cpp
    Source/Asio/AsioDebug.cpp

    # Generated
    ${GeneratedLibSchemaCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Bridge CXX Include Source)

# Includes
target_include_directories(GRS.Libraries.Bridge PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Setup dependencies
ExternalProject_Link(GRS.Libraries.Bridge Asio)

# Links
target_link_libraries(GRS.Libraries.Bridge PUBLIC GRS.Libraries.Message)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Libraries.Bridge PRIVATE /EHa)
endif()

#----- Tests -----#

# Create test layer
add_executable(
    GRS.Libraries.Bridge.Tests
    Tests/Source/Main.cpp
    Tests/Source/Emitter.cpp
    Tests/Source/Asio.cpp
)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Libraries.Bridge.Tests PRIVATE /EHa)
endif()

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Bridge.Tests CXX Tests)

# Includes
target_include_directories(GRS.Libraries.Bridge.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(GRS.Libraries.Bridge.Tests Catch2)

# Links
target_link_libraries(GRS.Libraries.Bridge.Tests PUBLIC GRS.Libraries.Bridge)

#----- .Net bindings -----#

if (${BUILD_UIX})
    include(DotNet.cmake)
endif()
