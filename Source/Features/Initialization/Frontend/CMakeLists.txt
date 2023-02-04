# Export stability feature library

#----- .Net UIX -----#

if (${BUILD_UIX})
    # Add external projects
    include_external_msproject(GRS.Features.Initialization.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(GRS.Features.Initialization.Frontend.UIX PROPERTIES FOLDER Source/Features/Initialization/Frontend)

    # Add runtime dependencies
    add_dependencies(
        GRS.Features.Initialization.Frontend.UIX
        GRS.Features.Initialization.Backend.DotNet
        UIX.Runtime
    )
endif()
