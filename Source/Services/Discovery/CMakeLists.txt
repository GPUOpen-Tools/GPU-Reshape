
# Create library
add_library(
    Services.Discovery STATIC
    Source/DiscoveryService.cpp
)

# IDE source discovery
SetSourceDiscovery(Services.Discovery Include Source)

# Include directories
target_include_directories(Services.Discovery PUBLIC Include)

# Dependencies
target_link_libraries(Services.Discovery PUBLIC Libraries.Discovery Libraries.Bridge Libraries.Backend)

#------ Standalone ------#

# Create standalone
add_executable(
    Services.Discovery.Standalone
    Standalone/main.cpp
)

# IDE source discovery
SetSourceDiscovery(Services.Discovery.Standalone Standalone)

# Dependencies
target_link_libraries(Services.Discovery.Standalone PUBLIC Services.Discovery)

# Create dependencies on existing listeners, for convenience
add_dependencies(Services.Discovery.Standalone Backends.Vulkan.Discovery)
