
# Create generator
add_library(
    GRS.Libraries.Common STATIC
    Source/Assert.cpp
    Source/FileSystem.cpp
    Source/CrashHandler.cpp
    Source/GlobalUID.cpp
    Source/CRC.cpp
    Source/Plugin/PluginResolver.cpp
    Source/Dispatcher/ConditionVariable.cpp
    Source/Dispatcher/Mutex.cpp
    Source/Dispatcher/EventCounter.cpp
    Source/Dispatcher/Event.cpp
    Source/Dispatcher/DispatcherJobPool.cpp

    # For IDE's
    Include/Common/TemplateEngine.h
    Include/Common/IDHash.h
    Include/Common/Assert.h
    Include/Common/Dispatcher/Dispatcher.h
    Include/Common/Dispatcher/DispatcherWorker.h
    Include/Common/Dispatcher/DispatcherJobPool.h
    Include/Common/Dispatcher/DispatcherJob.h
    Include/Common/String.h
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Common CXX Include Source)

# Include directories
target_include_directories(GRS.Libraries.Common PUBLIC Include)

# Links
ExternalProject_Link(GRS.Libraries.Common TinyXML2 tinyxml2)
ExternalProject_Link(GRS.Libraries.Common UnorderedDense)
ExternalProject_Link(GRS.Libraries.Common BTree)
ExternalProject_Link(GRS.Libraries.Common ZLIB $<$<CONFIG:Debug>:zlibstaticd> $<$<CONFIG:Release>:zlibstatic> $<$<CONFIG:RelWithDebInfo>:zlibstatic>)
ExternalProject_Link(GRS.Libraries.Common Fmt $<$<CONFIG:Debug>:fmtd> $<$<CONFIG:Release>:fmt> $<$<CONFIG:RelWithDebInfo>:fmt>)
