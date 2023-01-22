
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(Features.Initialization)
add_dependencies(
    Features.Initialization
    Features.Initialization.Backend
)

# IDE source discovery
SetSourceDiscovery(Features.Initialization CXX)

# Copy xml
ConfigureOutput(Initialization.xml Plugins/Initialization.xml)
