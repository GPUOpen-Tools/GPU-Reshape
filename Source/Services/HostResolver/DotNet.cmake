
# Create discovery
Project_AddDotNetEx(
    NAME GRS.Services.HostResolver.DotNet
    LANG CXX
    SOURCE
        Source/Managed/HostResolverService.cpp
    ASSEMBLIES
        System
        System.Core
        WindowsBase
    LIBS
        GRS.Services.HostResolver
    FLAGS
        /Zc:twoPhase-
)
