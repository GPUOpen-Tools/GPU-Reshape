
# Create generator
add_library(
    GRS.Libraries.Discovery STATIC
    Source/Discovery.cpp
    Source/DiscoveryHost.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Discovery CXX Include Source)

# Include directories
target_include_directories(GRS.Libraries.Discovery PUBLIC Include)

# Dependencies
target_link_libraries(GRS.Libraries.Discovery PUBLIC GRS.Libraries.Common)
