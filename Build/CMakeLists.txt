
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    include(VisualStudio.cmake)
endif()

function(SetSourceDiscovery NAME)
    if (CMAKE_GENERATOR MATCHES "Visual Studio")
        SetVisualStudioSourceDiscovery(${NAME} ${ARGN})
    endif()
endfunction(SetSourceDiscovery)
