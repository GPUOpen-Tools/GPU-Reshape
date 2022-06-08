
add_library(
    Libraries.Message.DotNet SHARED
    Source/Managed/MessageContainers.cs
    Source/Managed/MessageStream.cs
)

# Includes
target_include_directories(Libraries.Message.DotNet PUBLIC Include)

# DotNet
set_target_properties(Libraries.Message.DotNet PROPERTIES DOTNET_SDK "Microsoft.NET.Sdk")

# IDE source discovery
SetSourceDiscovery(Libraries.Message.DotNet CXX Include Source)
