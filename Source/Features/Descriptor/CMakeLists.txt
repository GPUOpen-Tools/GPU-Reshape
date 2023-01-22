
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(Features.Descriptor)
add_dependencies(
    Features.Descriptor
    Features.Descriptor.Backend
)

# IDE source discovery
SetSourceDiscovery(Features.Descriptor CXX)

# Copy xml
ConfigureOutput(Descriptor.xml Plugins/Descriptor.xml)
