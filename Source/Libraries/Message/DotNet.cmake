
add_library(
    Libraries.Message.DotNet SHARED
    Source/Managed/MessageContainers.cs
    Source/Managed/MessageStream.cs
    Source/Managed/MessageSchema.cs
    Source/Managed/MessageSubStream.cs
    Source/Managed/ByteSpan.cs

    # Generated
    ${GeneratedSchemaCS}
)

# Includes
target_include_directories(Libraries.Message.DotNet PUBLIC Include)

# DotNet
target_compile_options(Libraries.Message.DotNet PUBLIC "/unsafe")
set_property(TARGET Libraries.Message.DotNet PROPERTY VS_DOTNET_REFERENCES "System;System.Runtime;System.Memory")
set_target_properties(Libraries.Message.DotNet PROPERTIES DOTNET_SDK "Microsoft.NET.Sdk")

# IDE source discovery
SetSourceDiscovery(Libraries.Message.DotNet CS Include Source)

# Quick message project
function(Project_AddSchemaDotNet NAME)
    Project_AddDotNet(${NAME} ${ARGN})

    # Link to message
    set_property(TARGET ${NAME} PROPERTY VS_DOTNET_REFERENCES "System;System.Memory;Libraries.Message.DotNet")

    # Add explicit dependencies
    add_dependencies(${NAME} Libraries.Message.DotNet)
endfunction()
