
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(GRS.Features.Initialization)
add_dependencies(
    GRS.Features.Initialization
    GRS.Features.Initialization.Backend
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Initialization CXX)

# Copy xml
ConfigureOutput(Initialization.xml Plugins/Initialization.xml)
