
# Create library
add_library(
    Services.Discovery STATIC
    Source/DiscoveryService.cpp
)

# IDE source discovery
SetSourceDiscovery(Services.Discovery CXX Include Source)

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
SetSourceDiscovery(Services.Discovery.Standalone CXX Standalone)

# Dependencies
target_link_libraries(Services.Discovery.Standalone PUBLIC Services.Discovery)

# Create dependencies on existing listeners, for convenience
if (${ENABLE_BACKEND_VULKAN})
    add_dependencies(Services.Discovery.Standalone Backends.Vulkan.Discovery)
endif()
if (${ENABLE_BACKEND_DX12})
    add_dependencies(Services.Discovery.Standalone Backends.DX12.Discovery)
endif()

# Pull in all features, for convenience
add_dependencies(Services.Discovery.Standalone Features)

#----- .Net bindings -----#

if (${ENABLE_UIX})
    include(DotNet.cmake)
endif()
