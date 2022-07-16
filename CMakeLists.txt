project(gpu-validation)

cmake_minimum_required(VERSION 3.17)

# Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# All options
option(INSTALL_THIRD_PARTY   "Fetch and install third party libraries" ON)
option(ENABLE_BACKEND_VULKAN "Enables Vulkan backend" ON)
option(ENABLE_BACKEND_DX12   "Enables DX12 backend" ON)
option(ENABLE_UIX            "Enables UIX building (Experimental)" OFF)

# Developmental options
option(ENABLE_DXIL_DUMP      "Enables DXIL dumping on shader builds" OFF)
option(ENABLE_ASAN           "Enables ASAN" OFF)
option(ENABLE_RELEASE_DEBUG  "Enables debug information in release" ON)

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

# UIX bindings
if (${ENABLE_UIX})
    if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
        # CMake limitation
        message(FATAL_ERROR "UIX building currently only supported on Visual Studio generators")
    endif()

    # Enable additional languages
    enable_language(CSharp)

    # .NET 5.0 unfortunately has several issues with cmake
    set(CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION  "v4.7.2")
endif()

# Release configuration
if (CMAKE_BUILD_TYPE MATCHES "Release")
    if (MSVC)
        # Ensure executables share the same runtimes
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD")

        # Debug info?
        if (ENABLE_RELEASE_DEBUG)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Z7 /DEBUG")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
        endif()
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    endif()
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

# Post build copy for multi-configurators
function(PostBuildCopy NAME DIR)
    add_custom_command(
        TARGET ${NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${NAME}> $<TARGET_FILE_DIR:${NAME}>/${DIR}/$<TARGET_FILE_NAME:${NAME}>
    )
endfunction()

# Add all projects
add_subdirectory(Build)
add_subdirectory(ThirdParty)
add_subdirectory(Source)
