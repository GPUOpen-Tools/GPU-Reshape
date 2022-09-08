
add_library(
    Services.Discovery.DotNet SHARED
    Source/Managed/DiscoveryService.cpp
)

# Includes
target_include_directories(Services.Discovery.DotNet PUBLIC Include)

# Links
target_link_libraries(Services.Discovery.DotNet PUBLIC Services.Discovery)

# Resolver workaround
target_compile_options(Services.Discovery.DotNet PRIVATE /Zc:twoPhase-)

# DotNet
set_target_properties(Services.Discovery.DotNet PROPERTIES DOTNET_SDK "Microsoft.NET.Sdk")
set_target_properties(Services.Discovery.DotNet PROPERTIES VS_DOTNET_REFERENCES "System;System.Core;WindowsBase")
set_target_properties(Services.Discovery.DotNet PROPERTIES COMMON_LANGUAGE_RUNTIME "")

# IDE source discovery
SetSourceDiscovery(Services.Discovery.DotNet CXX Include Source)
