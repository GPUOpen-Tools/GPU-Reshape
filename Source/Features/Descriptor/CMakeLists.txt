
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(GRS.Features.Descriptor)
add_dependencies(
    GRS.Features.Descriptor
    GRS.Features.Descriptor.Backend
)

# IDE source discovery
SetSourceDiscovery(GRS.Features.Descriptor CXX)

# Copy xml
ConfigureOutput(Descriptor.xml Plugins/Descriptor.xml)
