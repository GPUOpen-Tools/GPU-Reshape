# Create DotNet project
Project_AddDotNetEx(
    NAME Libraries.Message.DotNet
    LANG CS
    GENERATED
        GeneratedSchemaCS
    SOURCE
        Source/Managed/MessageContainers.cs
        Source/Managed/MessageStream.cs
        Source/Managed/MessageSchema.cs
        Source/Managed/MessageSubStream.cs
        Source/Managed/ByteSpan.cs
    ASSEMBLIES
        System
        System.Runtime
        System.Memory
    FLAGS
        /unsafe
)

# Quick message project
function(Project_AddSchemaDotNet NAME SCHEMA_GEN)
    Project_AddDotNetEx(
        NAME ${NAME}
        LANG CS
        GENERATED
            "${SCHEMA_GEN}"
        ASSEMBLIES
            System
            System.Memory
        LIBS
            Libraries.Message.DotNet
    )
endfunction()
