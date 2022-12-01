
# Common set of utilities
add_subdirectory(Common)

# Plugins
add_subdirectory(ResourceBounds)
add_subdirectory(ExportStability)
add_subdirectory(Concurrency)
add_subdirectory(Descriptor)

# Experimental plugins
if (${ENABLE_EXPERIMENTAL})
    add_subdirectory(Initialization)
endif()

# Proxy object
add_custom_target(Features)
add_dependencies(
    Features
    Features.ResourceBounds
)
