
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(GRS.Features.ExportStability)
add_dependencies(
    GRS.Features.ExportStability
    GRS.Features.ExportStability.Backend
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.ExportStability CXX)

# Copy xml
ConfigureOutput(ExportStability.xml Plugins/ExportStability.xml)
