
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(GRS.Features.Loop)
add_dependencies(
    GRS.Features.Loop
    GRS.Features.Loop.Backend
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Loop CXX)

# Copy xml
ConfigureOutput(Loop.xml Plugins/Loop.xml)
