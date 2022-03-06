# Resource bounds feature library

# Create feature
add_library(
    Features.ResourceBounds.Frontend SHARED
    Source/Listener.cpp
    Source/Plugin.cpp
)

# IDE source discovery
SetSourceDiscovery(Features.ResourceBounds.Frontend Include Source)

# Includes
target_include_directories(Features.ResourceBounds.Frontend PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Features.ResourceBounds.Frontend PUBLIC Features.ResourceBounds.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET Features.ResourceBounds.Frontend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Features.ResourceBounds.Frontend> $<TARGET_FILE_DIR:Features.ResourceBounds.Frontend>/Plugins/$<TARGET_FILE_NAME:Features.ResourceBounds.Frontend>
)
