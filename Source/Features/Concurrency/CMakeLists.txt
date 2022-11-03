
# Libraries
add_subdirectory(Backend)
add_subdirectory(Frontend)

# Proxy object
add_custom_target(Features.Concurrency)
add_dependencies(
    Features.Concurrency
    Features.Concurrency.Backend
)

# Copy xml
ConfigureOutput(Concurrency.xml Plugins/Concurrency.xml)
