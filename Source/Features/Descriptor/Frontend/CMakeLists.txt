# Export stability feature library

#----- .Net UIX -----#

if (${ENABLE_UIX})
    # Add external projects
    include_external_msproject(Features.Descriptor.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(Features.Descriptor.Frontend.UIX PROPERTIES FOLDER Source/Features/Descriptor/Frontend)

    # Add runtime dependencies
    add_dependencies(
        Features.Descriptor.Frontend.UIX
        Features.Descriptor.Backend.DotNet
        UIX.Runtime
    )
endif()
