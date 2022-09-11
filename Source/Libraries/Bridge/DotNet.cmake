
# Create schema
Project_AddSchemaDotNet(Libraries.Bridge.Schema.DotNet GeneratedLibSchemaCS)

# Create bridge
Project_AddDotNetEx(
    NAME Libraries.Bridge.DotNet
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
        Libraries.Bridge
        Libraries.Message.DotNet
    FLAGS
        /Zc:twoPhase-
)
