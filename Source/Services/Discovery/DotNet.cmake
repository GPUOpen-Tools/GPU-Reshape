
# Create discovery
Project_AddDotNetEx(
    NAME Services.Discovery.DotNet
    LANG CXX
    SOURCE
        Source/Managed/DiscoveryService.cpp
    ASSEMBLIES
        System
        System.Core
        WindowsBase
    LIBS
        Services.Discovery
    FLAGS
        /Zc:twoPhase-
)
