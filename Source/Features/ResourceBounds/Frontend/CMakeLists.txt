# Resource bounds feature library

#----- Hub -----#

# Create feature
add_library(
    Features.ResourceBounds.Frontend.Hub SHARED
    Hub/Source/Listener.cpp
    Hub/Source/Plugin.cpp
)

# IDE source discovery
SetSourceDiscovery(Features.ResourceBounds.Frontend.Hub CXX Hub/Include Hub/Source)

# Includes
target_include_directories(Features.ResourceBounds.Frontend.Hub PUBLIC Hub/Include ${CMAKE_CURRENT_BINARY_DIR}/Hub/Include)

# Links
target_link_libraries(Features.ResourceBounds.Frontend.Hub PUBLIC Features.ResourceBounds.Backend)

# Set all outputs to Plugins/.
add_custom_command(
    TARGET Features.ResourceBounds.Frontend.Hub POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Features.ResourceBounds.Frontend.Hub> $<TARGET_FILE_DIR:Features.ResourceBounds.Frontend.Hub>/Plugins/$<TARGET_FILE_NAME:Features.ResourceBounds.Frontend.Hub>
)

#----- .Net UIX -----#

if (${ENABLE_UIX})
    # Add external projects
    include_external_msproject(Features.ResourceBounds.Frontend.UIX ${CMAKE_CURRENT_SOURCE_DIR}/UIX/UIX.csproj)

    # IDE discovery
    set_target_properties(Features.ResourceBounds.Frontend.UIX PROPERTIES FOLDER Source/Features/ResourceBounds/Frontend)

    # Add runtime dependencies
    add_dependencies(
        Features.ResourceBounds.Frontend.UIX
        Features.ResourceBounds.Backend.DotNet
        UIX.Runtime
    )
endif()
