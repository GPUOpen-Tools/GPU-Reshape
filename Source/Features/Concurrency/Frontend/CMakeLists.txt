# Export stability feature library

#----- .Net UIX -----#

if (${ENABLE_UIX})
    # Add external projects
    include_external_msproject(Features.Concurrency.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(Features.Concurrency.Frontend.UIX PROPERTIES FOLDER Source/Features/Concurrency/Frontend)

    # Add runtime dependencies
    add_dependencies(
        Features.Concurrency.Frontend.UIX
        Features.Concurrency.Backend.DotNet
        UIX.Runtime
    )
endif()
