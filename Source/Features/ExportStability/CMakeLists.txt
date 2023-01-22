
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(Features.ExportStability)
add_dependencies(
    Features.ExportStability
    Features.ExportStability.Backend
)

# IDE source discovery
SetSourceDiscovery(Features.ExportStability CXX)

# Copy xml
ConfigureOutput(ExportStability.xml Plugins/ExportStability.xml)
