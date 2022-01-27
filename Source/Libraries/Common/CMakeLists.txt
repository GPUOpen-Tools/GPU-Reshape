
# Create generator
add_library(
    Libraries.Common STATIC
    Source/Assert.cpp
    Source/Path.cpp
    Source/Plugin/PluginResolver.cpp

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

# Include directories
target_include_directories(Libraries.Common PUBLIC Include)

# Links
ExternalProject_Link(Libraries.Common TinyXML2 tinyxml2)

#----- Visual Studio -----#

# Folder structure
set_target_properties(Libraries.Common PROPERTIES FOLDER Libraries/Common)
