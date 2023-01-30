
#------ Standalone ------#

# Create standalone
add_executable(
    Services.HostResolver.Standalone
    Standalone/main.cpp
)

# Build convenience, kill the process
if (WIN32)
    add_custom_command(
        TARGET Services.HostResolver.Standalone
        PRE_BUILD COMMAND ${CMAKE_SOURCE_DIR}/Build/Utils/KillHostResolverService.bat
    )
endif()

# IDE source discovery
SetSourceDiscovery(Services.HostResolver.Standalone CXX Standalone)

# Include directories
target_include_directories(Services.HostResolver.Standalone PUBLIC Include)

# Dependencies
target_link_libraries(Services.HostResolver.Standalone PUBLIC Libraries.Bridge)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(Services.HostResolver.Standalone PRIVATE /EHa)
endif()

#------ Service ------#

# Create library
add_library(
    Services.HostResolver STATIC
    Source/HostResolverService.cpp
)

# IDE source discovery
SetSourceDiscovery(Services.HostResolver CXX Include Source)

# Include directories
target_include_directories(Services.HostResolver PUBLIC Include)

# Dependencies
target_link_libraries(Services.HostResolver PUBLIC Libraries.Bridge)

# Ensure the standalone is compiled
add_dependencies(Services.HostResolver Services.HostResolver.Standalone)

#----- .Net bindings -----#

if (${BUILD_UIX})
    include(DotNet.cmake)
endif()
