project(gpu-validation)

cmake_minimum_required(VERSION 3.19)

# Ensure 64 bit
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Supported
else()
    message(FATAL_ERROR "Only x64 target not supported")
endif()

# Toolchain support
if (MSVC)
    # Supported
elseif(MINGW)
    message(WARNING "MinGW support not complete")
else()
    message(FATAL_ERROR "Unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

set(CMAKE_CXX_STANDARD 17)

# Set all outputs to Bin/Config
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})

add_subdirectory(ThirdParty)
add_subdirectory(Tools)
add_subdirectory(Source)
