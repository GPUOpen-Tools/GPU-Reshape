
# Common set of utilities
add_subdirectory(Common)

# Plugins
add_subdirectory(ResourceBounds)
add_subdirectory(ExportStability)

# Experimental plugins
if (${ENABLE_EXPERIMENTAL})
    add_subdirectory(Concurrency)
endif()

# Proxy object
add_custom_target(Features)
add_dependencies(
    Features
    Features.ResourceBounds
)
