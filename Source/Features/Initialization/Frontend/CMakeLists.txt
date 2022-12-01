# Export stability feature library

#----- .Net UIX -----#

if (${ENABLE_UIX})
    # Add external projects
    include_external_msproject(Features.Initialization.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(Features.Initialization.Frontend.UIX PROPERTIES FOLDER Source/Features/Initialization/Frontend)

    # Add runtime dependencies
    add_dependencies(
        Features.Initialization.Frontend.UIX
        Features.Initialization.Backend.DotNet
        UIX.Runtime
    )
endif()
