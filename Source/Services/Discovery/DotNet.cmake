
# Create discovery
Project_AddDotNetEx(
    NAME GRS.Services.Discovery.DotNet
    LANG CXX
    SOURCE
        Source/Managed/DiscoveryService.cpp
    ASSEMBLIES
        System
        System.Core
        WindowsBase
    LIBS
        GRS.Services.Discovery
        GRS.Libraries.Message.DotNet
    FLAGS
        /Zc:twoPhase-
)

# Add external projects
include_external_msproject(GRS.Services.Discovery.NotifyIcon ${CMAKE_CURRENT_SOURCE_DIR}/NotifyIcon/NotifyIcon.csproj)

# IDE discovery
set_target_properties(GRS.Services.Discovery.NotifyIcon PROPERTIES FOLDER Source/Services/Discovery)

# Add runtime dependencies
add_dependencies(
    GRS.Services.Discovery.NotifyIcon
    GRS.Services.Discovery.DotNet
)
