# Resource bounds feature library

# Generate the schema
Project_AddShaderSchema(Generated Schemas/ResourceBounds.xml Include/Schemas/Features/ResourceBounds)

# Create feature
add_library(
    Features.ResourceBounds SHARED
    Source/Feature.cpp

    # Generated
    ${Generated}
)

# Includes
target_include_directories(Features.ResourceBounds PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Features.ResourceBounds Libraries.Common Libraries.Backend)
