
# Get all includes
file(GLOB Sources LibClang/include/*)

# Configure all includes
foreach (File ${Sources})
    get_filename_component(BaseName "${File}" NAME)
    configure_file(${File} ${CMAKE_BINARY_DIR}/External/include/clang-c/${BaseName})
endforeach ()

# Copy libs
if (WIN32)
    configure_file(LibClang/lib/Win64/libclang.lib ${CMAKE_BINARY_DIR}/External/lib/libclang.lib COPYONLY)
endif()

# Copy binaries
if (WIN32)
    ConfigureOutput(LibClang/bin/Win64/libclang.dll libclang.dll)
endif()
