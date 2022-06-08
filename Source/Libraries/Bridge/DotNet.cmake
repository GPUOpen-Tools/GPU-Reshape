
add_library(
    Libraries.Bridge.DotNet SHARED
    Source/Managed/IBridge.cpp
    Source/Managed/RemoteClientBridge.cpp
)

# Includes
target_include_directories(Libraries.Bridge.DotNet PUBLIC Include)

# Links
target_link_libraries(Libraries.Bridge.DotNet PUBLIC Libraries.Bridge Libraries.Message.DotNet)

# DotNet
set_target_properties(Libraries.Bridge.DotNet PROPERTIES DOTNET_SDK "Microsoft.NET.Sdk")
set_target_properties(Libraries.Bridge.DotNet PROPERTIES VS_DOTNET_REFERENCES "System;System.Core;WindowsBase;Libraries.Message.DotNet")
set_target_properties(Libraries.Bridge.DotNet PROPERTIES COMMON_LANGUAGE_RUNTIME "")

# IDE source discovery
SetSourceDiscovery(Libraries.Bridge.DotNet CXX Include Source)
