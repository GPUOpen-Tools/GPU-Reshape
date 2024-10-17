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

# x64 specific sources
if (NOT THIN_X86_BUILD)
    set(
        CommonX64Sources
        Source/CRC.cpp
        Source/Plugin/PluginResolver.cpp
    )
endif()

# Create generator
add_library(
    GRS.Libraries.Common STATIC
    Source/Assert.cpp
    Source/FileSystem.cpp
    Source/CrashHandler.cpp
    Source/GlobalUID.cpp
    Source/Library.cpp
    Source/Dispatcher/ConditionVariable.cpp
    Source/Dispatcher/Mutex.cpp
    Source/Dispatcher/EventCounter.cpp
    Source/Dispatcher/Event.cpp
    Source/Dispatcher/DispatcherJobPool.cpp
    Source/Containers/ReferenceObject.cpp

    # For IDE's
    Include/Common/TemplateEngine.h
    Include/Common/IDHash.h
    Include/Common/Assert.h
    Include/Common/Dispatcher/Dispatcher.h
    Include/Common/Dispatcher/DispatcherWorker.h
    Include/Common/Dispatcher/DispatcherJobPool.h
    Include/Common/Dispatcher/DispatcherJob.h
    Include/Common/String.h
    
    # x64
    ${CommonX64Sources}
)

# Include directories
target_include_directories(GRS.Libraries.Common PUBLIC Include)

# Rest is x64 only
if (THIN_X86_BUILD)
    return()
endif()

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Common CXX Include Source)

# Links
ExternalProject_Link(GRS.Libraries.Common TinyXML2 tinyxml2)
ExternalProject_Link(GRS.Libraries.Common UnorderedDense)
ExternalProject_Link(GRS.Libraries.Common BTree)
ExternalProject_Link(GRS.Libraries.Common ZLIB $<$<CONFIG:Debug>:zlibstaticd> $<$<CONFIG:Release>:zlibstatic> $<$<CONFIG:RelWithDebInfo>:zlibstatic>)
ExternalProject_Link(GRS.Libraries.Common Fmt $<$<CONFIG:Debug>:fmtd> $<$<CONFIG:Release>:fmt> $<$<CONFIG:RelWithDebInfo>:fmt>)
