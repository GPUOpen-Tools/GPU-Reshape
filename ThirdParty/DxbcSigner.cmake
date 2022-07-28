
# Get all includes
file(GLOB Sources DxbcSigner/include/*)

# Configure all includes
foreach (File ${Sources})
    get_filename_component(BaseName "${File}" NAME)
    configure_file(${File} ${CMAKE_BINARY_DIR}/External/include/DxbcSigner/${BaseName})
endforeach ()

# Copy libs
if (WIN32)
    configure_file(DxbcSigner/bin/win64/dxbcSigner.lib ${CMAKE_BINARY_DIR}/External/lib/dxbcSigner.lib COPYONLY)
endif()

# Copy binaries
if (WIN32)
    ConfigureOutput(DxbcSigner/bin/win64/dxbcSigner.dll dxbcSigner.dll)
endif()
