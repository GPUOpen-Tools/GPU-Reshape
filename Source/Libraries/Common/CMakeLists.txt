
# Create generator
add_library(
    Libraries.Common STATIC
    Source/Assert.cpp
    Source/FileSystem.cpp
    Source/CrashHandler.cpp
    Source/GlobalUID.cpp
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
SetSourceDiscovery(Libraries.Common CXX Include Source)

# Include directories
target_include_directories(Libraries.Common PUBLIC Include)

# Links
ExternalProject_Link(Libraries.Common TinyXML2 tinyxml2)
ExternalProject_Link(Libraries.Common Fmt $<$<CONFIG:Debug>:fmtd> $<$<CONFIG:Release>:fmt>)
