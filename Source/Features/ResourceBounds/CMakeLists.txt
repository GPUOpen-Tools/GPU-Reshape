
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(Features.ResourceBounds)
add_dependencies(
    Features.ResourceBounds
    Features.ResourceBounds.Backend
    Features.ResourceBounds.Frontend.Hub
)

# Copy xml
ConfigureOutput(ResourceBounds.xml Plugins/ResourceBounds.xml)
