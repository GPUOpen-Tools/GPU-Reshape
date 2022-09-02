
# Libraries
add_subdirectory(Backend)

# Proxy object
add_custom_target(Features.ExportStability)
add_dependencies(
    Features.ExportStability
    Features.ExportStability.Backend
)

# Copy xml
ConfigureOutput(ExportStability.xml Plugins/ExportStability.xml)
