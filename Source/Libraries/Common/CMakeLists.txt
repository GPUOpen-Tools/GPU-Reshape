
# Create generator
add_library(
    Libraries.Common OBJECT
    Source/Assert.cpp

    # For IDE's
    Include/Common/TemplateEngine.h
    Include/Common/IDHash.h
    Include/Common/Assert.h
    Include/Common/Dispatcher.h
    Include/Common/DispatcherWorker.h
    Include/Common/DispatcherJobPool.h
    Include/Common/DispatcherJob.h
    Include/Common/Trim.h
)

# Include directories
target_include_directories(Libraries.Common PUBLIC Include)
