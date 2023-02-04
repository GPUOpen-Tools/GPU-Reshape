
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(GRS.Features.Concurrency)
add_dependencies(
    GRS.Features.Concurrency
    GRS.Features.Concurrency.Backend
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Concurrency CXX)

# Copy xml
ConfigureOutput(Concurrency.xml Plugins/Concurrency.xml)
