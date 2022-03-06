
# Common set of utilities
add_subdirectory(Common)

# Plugins
add_subdirectory(ResourceBounds)

# Proxy object
add_custom_target(Features)
add_dependencies(
    Features
    Features.ResourceBounds
)
