# Quick library setup
function(Project_AddDotNet NAME)
    add_library(
        ${NAME} SHARED
        ${ARGN}
    )

    # DotNet
    set_target_properties(${NAME} PROPERTIES DOTNET_SDK "Microsoft.NET.Sdk")

    # IDE source discovery
    SetSourceDiscovery(${NAME} CS Include Source Schema)
endfunction()
