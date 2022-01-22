
add_subdirectory(Backend)

configure_file(ResourceBounds.xml ${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/Plugins/ResourceBounds.xml)
