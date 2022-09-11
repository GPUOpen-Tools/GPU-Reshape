
# Create discovery
Project_AddDotNetEx(
    NAME Services.HostResolver.DotNet
    LANG CXX
    SOURCE
        Source/Managed/HostResolverService.cpp
    ASSEMBLIES
        System
        System.Core
        WindowsBase
    LIBS
        Services.HostResolver
    FLAGS
        /Zc:twoPhase-
)
