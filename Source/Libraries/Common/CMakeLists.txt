
# Create generator
add_library(
    Libraries.Common OBJECT
    Include/Common/TemplateEngine.h
)

# Include directories
target_include_directories(Libraries.Common PUBLIC Include)

# Set language of object lib
set_target_properties(Libraries.Common PROPERTIES LINKER_LANGUAGE CXX)
