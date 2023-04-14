
# Create library
add_library(
    GRS.Services.Discovery STATIC
    Source/DiscoveryService.cpp
    Source/NotifyIconDiscoveryListener.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Services.Discovery CXX Include Source)

# Include directories
target_include_directories(GRS.Services.Discovery PUBLIC Include)

# Dependencies
target_link_libraries(GRS.Services.Discovery PUBLIC GRS.Libraries.Discovery GRS.Libraries.Bridge GRS.Libraries.Backend)

# External
ExternalProject_Link(GRS.Services.Discovery Detour Detour)

#------ Standalone ------#

# Create standalone
add_executable(
    GRS.Services.Discovery.Standalone
    Standalone/main.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Services.Discovery.Standalone CXX Standalone)

# Dependencies
target_link_libraries(GRS.Services.Discovery.Standalone PUBLIC GRS.Services.Discovery)

# Create dependencies on existing listeners, for convenience
if (${ENABLE_BACKEND_VULKAN})
    add_dependencies(GRS.Services.Discovery.Standalone GRS.Backends.Vulkan.Discovery)
endif()
if (${ENABLE_BACKEND_DX12})
    add_dependencies(GRS.Services.Discovery.Standalone GRS.Backends.DX12.Discovery)
endif()

# Pull in all features, for convenience
add_dependencies(GRS.Services.Discovery.Standalone Features)

#----- .Net bindings -----#

if (${BUILD_UIX})
    include(DotNet.cmake)
endif()
