
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
    FLAGS
        /Zc:twoPhase-
)
