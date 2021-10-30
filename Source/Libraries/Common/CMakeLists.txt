
# Create generator
add_library(
    Libraries.Common OBJECT
    Source/Assert.cpp

    # For IDE's
    Include/Common/TemplateEngine.h
    Include/Common/IDHash.h
    Include/Common/Assert.h
)

# Include directories
target_include_directories(Libraries.Common PUBLIC Include)
