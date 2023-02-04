
# Create schema
Project_AddSchemaDotNet(GRS.Libraries.Bridge.Schema.DotNet GeneratedLibSchemaCS)

# Create bridge
Project_AddDotNetEx(
    NAME GRS.Libraries.Bridge.DotNet
    LANG CXX
    SOURCE
        Source/Managed/IBridge.cpp
        Source/Managed/BridgeMessageStorage.cpp
        Source/Managed/RemoteClientBridge.cpp
    ASSEMBLIES
        System
        System.Core
        WindowsBase
    LIBS
        GRS.Libraries.Bridge
        GRS.Libraries.Message.DotNet
    FLAGS
        /Zc:twoPhase-
)
