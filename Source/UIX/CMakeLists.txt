
# Add external projects
include_external_msproject(UIX.Runtime ${CMAKE_CURRENT_SOURCE_DIR}/Runtime/Runtime.csproj)
include_external_msproject(UIX.Studio ${CMAKE_CURRENT_SOURCE_DIR}/Studio/Studio.csproj)

# IDE discovery
set_target_properties(UIX.Runtime PROPERTIES FOLDER Source/UIX/Runtime)
set_target_properties(UIX.Studio PROPERTIES FOLDER Source/UIX/Studio)

# Add runtime dependencies
add_dependencies(
    UIX.Runtime
    GRS.Libraries.Bridge.DotNet
    GRS.Libraries.Message.DotNet
    GRS.Services.HostResolver.DotNet
    GRS.Services.Discovery.DotNet
    GRS.Libraries.Bridge.Schema.DotNet
    GRS.Libraries.Backend.Schema.DotNet
)

# Add studio dependencies
add_dependencies(
    UIX.Studio
    UIX.Runtime
    GRS.Services.Discovery.NotifyIcon
)
