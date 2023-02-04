
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(GRS.Features.ResourceBounds)
add_dependencies(
    GRS.Features.ResourceBounds
    GRS.Features.ResourceBounds.Backend
    GRS.Features.ResourceBounds.Frontend.Hub
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.ResourceBounds CXX)

# Copy xml
ConfigureOutput(ResourceBounds.xml Plugins/ResourceBounds.xml)
