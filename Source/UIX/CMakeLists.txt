
# Add external library
include_external_msproject(UIX.Studio ${CMAKE_CURRENT_SOURCE_DIR}/Studio/Studio.csproj)

# Add dependencies
add_dependencies(
    UIX.Studio
    Features.ResourceBounds.Backend.DotNet
    Libraries.Bridge.DotNet
    Libraries.Message.DotNet
    Services.HostResolver.DotNet
    Services.Discovery.DotNet
    Libraries.Bridge.Schema.DotNet
    Libraries.Backend.Schema.DotNet
)

# IDE discovery
set_target_properties(UIX.Studio PROPERTIES FOLDER UIX/Studio)
