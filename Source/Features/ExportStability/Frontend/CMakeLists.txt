# Export stability feature library

#----- .Net UIX -----#

if (${BUILD_UIX})
    # Add external projects
    include_external_msproject(GRS.Features.ExportStability.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(GRS.Features.ExportStability.Frontend.UIX PROPERTIES FOLDER Source/Features/ExportStability/Frontend)

    # Add runtime dependencies
    add_dependencies(
        GRS.Features.ExportStability.Frontend.UIX
        GRS.Features.ExportStability.Backend.DotNet
        UIX.Runtime
    )
endif()
