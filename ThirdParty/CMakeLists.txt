include(ExternalProject)

# Disconnected third party fetch?
if (INSTALL_THIRD_PARTY)
    set(ThirdPartyDisconnected 0)
else()
    set(ThirdPartyDisconnected 1)
endif()

# Global
set(ThirdPartyDisconnected ${ThirdPartyDisconnected} CACHE INTERNAL "ThirdPartyDisconnected")

# Install all third party libraries
# TODO: While the pulled objects are generally trusted, there should be some safety mechanism
include(Catch2.cmake)
include(SPIRV.cmake)
include(Vulkan.cmake)
include(ArgParser.cmake)
include(TinyXML2.cmake)
include(VMA.cmake)
include(Asio.cmake)
include(Fmt.cmake)
include(JSON.cmake)
include(LibClang.cmake)
include(GraphViz.cmake)
include(DXC.cmake)
include(Detour.cmake)
include(DXIL.cmake)

# UI?
if (${ENABLE_UIX})
    include(Avalonia.cmake)
endif ()

# Standard project installation
function(ExternalProject_Link Project Name)
    if (TARGET ${Name})
        add_dependencies(${Project} ${Name})
    endif()

    target_include_directories(${Project} PUBLIC ${CMAKE_BINARY_DIR}/External/include)
    target_link_directories(${Project} PUBLIC ${CMAKE_BINARY_DIR}/External/lib)

    if (NOT "${ARGN}" STREQUAL "")
        target_link_libraries(${Project} PUBLIC ${ARGN})
    endif()
endfunction()
