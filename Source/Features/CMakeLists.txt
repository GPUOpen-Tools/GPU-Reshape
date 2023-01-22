
# Common set of utilities
add_subdirectory(Common)

# Plugins
add_subdirectory(ResourceBounds)
add_subdirectory(ExportStability)
add_subdirectory(Concurrency)
add_subdirectory(Descriptor)
add_subdirectory(Initialization)

# Proxy object
add_custom_target(Features)
add_dependencies(
    Features
    Features.ResourceBounds
)

# IDE source discovery
SetSourceDiscovery(Features CXX)
