# Export stability feature library

#----- .Net UIX -----#

if (${BUILD_UIX})
    # Add external projects
    include_external_msproject(GRS.Features.Loop.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(GRS.Features.Loop.Frontend.UIX PROPERTIES FOLDER Source/Features/Loop/Frontend)

    # Add runtime dependencies
    add_dependencies(
        GRS.Features.Loop.Frontend.UIX
        GRS.Features.Loop.Backend.DotNet
        UIX.Runtime
    )
endif()
