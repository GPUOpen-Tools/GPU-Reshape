
#------ Standalone ------#

# Create standalone
add_executable(
    GRS.Services.HostResolver.Standalone
    Standalone/main.cpp
)

# Build convenience, kill the process
if (WIN32)
    add_custom_command(
        TARGET GRS.Services.HostResolver.Standalone
        PRE_BUILD COMMAND ${CMAKE_SOURCE_DIR}/Build/Utils/KillHostResolverService.bat
    )
endif()

# IDE source discovery
SetSourceDiscovery(GRS.Services.HostResolver.Standalone CXX Standalone)

# Include directories
target_include_directories(GRS.Services.HostResolver.Standalone PUBLIC Include)

# Dependencies
target_link_libraries(GRS.Services.HostResolver.Standalone PUBLIC GRS.Libraries.Bridge)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Services.HostResolver.Standalone PRIVATE /EHa)
endif()

#------ Service ------#

# Create library
add_library(
    GRS.Services.HostResolver STATIC
    Source/HostResolverService.cpp
)

# IDE source discovery
SetSourceDiscovery(GRS.Services.HostResolver CXX Include Source)

# Include directories
target_include_directories(GRS.Services.HostResolver PUBLIC Include)

# Dependencies
target_link_libraries(GRS.Services.HostResolver PUBLIC GRS.Libraries.Bridge)

# Ensure the standalone is compiled
add_dependencies(GRS.Services.HostResolver GRS.Services.HostResolver.Standalone)

#----- .Net bindings -----#

if (${BUILD_UIX})
    include(DotNet.cmake)
endif()
