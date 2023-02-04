# Export stability feature library

#----- .Net UIX -----#

if (${BUILD_UIX})
    # Add external projects
    include_external_msproject(GRS.Features.Concurrency.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(GRS.Features.Concurrency.Frontend.UIX PROPERTIES FOLDER Source/Features/Concurrency/Frontend)

    # Add runtime dependencies
    add_dependencies(
        GRS.Features.Concurrency.Frontend.UIX
        GRS.Features.Concurrency.Backend.DotNet
        UIX.Runtime
    )
endif()
