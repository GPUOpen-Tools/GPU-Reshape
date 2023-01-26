
include(Files.cmake)

if (${ENABLE_UIX})
    include(CSharp.cmake)
endif()

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    include(VisualStudio.cmake)
    include(MSBuild.cmake)
endif()

if (${ENABLE_ASAN} AND NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    include(Asan.cmake)
endif()

function(SetSourceDiscovery NAME LANG)
    if (CMAKE_GENERATOR MATCHES "Visual Studio")
        SetVisualStudioSourceDiscovery(${NAME} ${LANG} ${ARGN})
    endif()
endfunction(SetSourceDiscovery)
