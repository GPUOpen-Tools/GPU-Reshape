project(gpu-validation)

cmake_minimum_required(VERSION 3.17)

# Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# All options
option(INSTALL_THIRD_PARTY "Fetch and install third party libraries" 0)

# Ensure 64 bit
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Supported
else()
    message(FATAL_ERROR "Only x64 target not supported")
endif()

# Toolchain support
if (MSVC)
    # Supported
    add_compile_options(/std:c++latest)
elseif(MINGW)
    message(WARNING "MinGW support not complete")

    # Set the standard
    set(CMAKE_CXX_STANDARD 17)
else()
    message(FATAL_ERROR "Unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

# Set all outputs to Bin/Config
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Lib/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Lib/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})

# Configure file for multi-configurators
function(ConfigureOutput FILE OUT)
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
        foreach(Config ${CMAKE_CONFIGURATION_TYPES})
            configure_file(${FILE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${Config}/${OUT} COPYONLY)
        endforeach()
    else()
        configure_file(${FILE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUT} COPYONLY)
    endif()
endfunction()

# Add all projects
add_subdirectory(ThirdParty)
add_subdirectory(Tools)
add_subdirectory(Source)
