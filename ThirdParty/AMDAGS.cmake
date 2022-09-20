
# Get all includes
file(GLOB Sources AMDAGS/include/*)

# Configure all includes
foreach (File ${Sources})
    get_filename_component(BaseName "${File}" NAME)
    configure_file(${File} ${CMAKE_BINARY_DIR}/External/include/AMDAGS/${BaseName})
endforeach ()
