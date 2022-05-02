
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    foreach(Config ${CMAKE_CONFIGURATION_TYPES})
        ConfigureOutputDirectory(${CMAKE_SOURCE_DIR}/ThirdParty/GraphViz/bin/Win64 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${Config}/GraphViz)
    endforeach()
else()
    ConfigureOutputDirectory(${CMAKE_SOURCE_DIR}/ThirdParty/GraphViz/bin/Win64 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/GraphViz)
endif()
