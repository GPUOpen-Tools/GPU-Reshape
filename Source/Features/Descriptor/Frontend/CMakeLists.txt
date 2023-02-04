# Export stability feature library

#----- .Net UIX -----#

if (${BUILD_UIX})
    # Add external projects
    include_external_msproject(GRS.Features.Descriptor.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(GRS.Features.Descriptor.Frontend.UIX PROPERTIES FOLDER Source/Features/Descriptor/Frontend)

    # Add runtime dependencies
    add_dependencies(
        GRS.Features.Descriptor.Frontend.UIX
        GRS.Features.Descriptor.Backend.DotNet
        UIX.Runtime
    )
endif()
