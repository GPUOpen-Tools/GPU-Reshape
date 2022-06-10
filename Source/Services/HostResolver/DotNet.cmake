
add_library(
    Services.HostResolver.DotNet SHARED
    Source/Managed/HostResolverService.cpp
)

# Includes
target_include_directories(Services.HostResolver.DotNet PUBLIC Include)

# Links
target_link_libraries(Services.HostResolver.DotNet PUBLIC Services.HostResolver)

# Resolver workaround
target_compile_options(Services.HostResolver.DotNet PRIVATE /Zc:twoPhase-)

# DotNet
set_target_properties(Services.HostResolver.DotNet PROPERTIES DOTNET_SDK "Microsoft.NET.Sdk")
set_target_properties(Services.HostResolver.DotNet PROPERTIES VS_DOTNET_REFERENCES "System;System.Core;WindowsBase")
set_target_properties(Services.HostResolver.DotNet PROPERTIES COMMON_LANGUAGE_RUNTIME "")

# IDE source discovery
SetSourceDiscovery(Services.HostResolver.DotNet CXX Include Source)
