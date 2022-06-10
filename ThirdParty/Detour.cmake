include(FetchContent)

# Detour, used for API hooking
#   ! Must match detour version
FetchContent_Declare(
    DetourFetch
    URL https://github.com/microsoft/Detours/zipball/v4.0.1
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Detour
)

# Pull at compile time
FetchContent_MakeAvailable(DetourFetch)

# Create layer
add_library(
    Detour STATIC
    Detour/src/creatwth.cpp
    Detour/src/detours.cpp
    Detour/src/disasm.cpp
    Detour/src/disolarm.cpp
    Detour/src/disolarm64.cpp
    Detour/src/disolia64.cpp
    Detour/src/disolx64.cpp
    Detour/src/disolx86.cpp
    Detour/src/image.cpp
    Detour/src/modules.cpp
)

# Get all includes
file(GLOB Includes Detour/src/*.h)

# Configure all includes
foreach (File ${Includes})
    get_filename_component(BaseName "${File}" NAME)
    configure_file(${File} ${CMAKE_BINARY_DIR}/External/include/Detour/${BaseName})
endforeach ()

# Set current version
#   ! Must match git version
target_compile_definitions(Detour PUBLIC -DDETOURS_VERSION=0x4c0c1)

# Compiler fixes
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Disable implicit pedantic mode set by standard
    get_target_property(DetourOptions Detour COMPILE_OPTIONS)
    list(REMOVE_ITEM DetourOptions "/std:c++20")
    set_property(TARGET Detour PROPERTY COMPILE_OPTIONS ${DetourOptions})
else()
    # Enable extensions
    target_compile_options(Detour PRIVATE -Wmicrosoft-goto)
endif()
