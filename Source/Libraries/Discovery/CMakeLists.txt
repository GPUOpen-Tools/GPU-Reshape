
# Create generator
add_library(
    Libraries.Discovery STATIC
    Source/Discovery.cpp
    Source/DiscoveryHost.cpp
)

# IDE source discovery
SetSourceDiscovery(Libraries.Discovery Include Source)

# Include directories
target_include_directories(Libraries.Discovery PUBLIC Include)

# Dependencies
target_link_libraries(Libraries.Discovery PUBLIC Libraries.Common)
