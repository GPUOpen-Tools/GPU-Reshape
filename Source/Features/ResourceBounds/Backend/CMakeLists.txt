# Resource bounds feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/ResourceBounds.xml Include/Schemas/Features/ResourceBounds)

# Create feature
add_library(
    Features.ResourceBounds.Backend SHARED
    Source/Feature.cpp
    Source/Plugin.cpp

    # Generated
    ${Generated}
)

# Includes
target_include_directories(Features.ResourceBounds.Backend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Features.ResourceBounds.Backend Libraries.Common Libraries.Backend)
