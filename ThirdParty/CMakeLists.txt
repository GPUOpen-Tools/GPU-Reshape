include(ExternalProject)

# Install all third party libraries
# TODO: While the pulled objects are generally trusted, there should be some safety mechanism
include(Catch2.cmake)
include(Vulkan.cmake)

# Standard project installation
function(ExternalProject_Link Project Name)
    add_dependencies(${Project} ${Name})

    target_include_directories(${Project} PUBLIC ${CMAKE_BINARY_DIR}/External/include)
    target_link_directories(${Project} PUBLIC ${CMAKE_BINARY_DIR}/External/lib)

    if (NOT "${ARGN}" STREQUAL "")
        target_link_libraries(${Project} ${ARGN})
    endif()
endfunction()
