# 
# The MIT License (MIT)
# 
# Copyright (c) 2024 Advanced Micro Devices, Inc.,
# Fatalist Development AB (Avalanche Studio Group),
# and Miguel Petersen.
# 
# All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy 
# of this software and associated documentation files (the "Software"), to deal 
# in the Software without restriction, including without limitation the rights 
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
# of the Software, and to permit persons to whom the Software is furnished to do so, 
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all 
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 

project(GPU-Reshape)

# Requirements
cmake_minimum_required(VERSION 3.24)

# Policies
cmake_policy(SET CMP0135 NEW)

# Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# All options
option(INSTALL_THIRD_PARTY     "Fetch and install third party libraries" ON)
option(ENABLE_BACKEND_VULKAN   "Enables Vulkan backend" ON)
option(ENABLE_BACKEND_DX12     "Enables DX12 backend" ON)
option(ENABLE_UIX              "Enables UIX building" ON)
option(ENABLE_MIXED_COMPILER   "Enables mixed compiler building, separates build folders" ON)
option(ENABLE_NUGET_RESTORE    "Enables nuget restore during configuration" ON)
option(ENABLE_X86_BOOTSTRAPPER "Enables the x86 bootstrapper" ON)
option(ENABLE_EXPERIMENTAL     "Enables experimental features" OFF)

# Developmental options
option(ENABLE_DXIL_DUMP      "Enables DXIL dumping on shader builds" ON)
option(ENABLE_ASAN           "Enables ASAN" OFF)
option(ENABLE_RELEASE_DEBUG  "Enables debug information in release" ON)

# Primary build tree?
if (NOT THIN_X86_BUILD AND ENABLE_X86_BOOTSTRAPPER)
    # CMake binary / cache folder
    set(ThinBinaryDir "${CMAKE_BINARY_DIR}/ThinX86")
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${ThinBinaryDir}")

    # Architecture and bootstrapper
    if (NOT "${CMAKE_GENERATOR}" MATCHES "Ninja")
        # Let CMake handle the x86 architecture
        set(X86CommandBootstrapper ${CMAKE_COMMAND})
        set(X86ArchFlags           -A Win32 -DCMAKE_GENERATOR_PLATFORM=Win32)
    else()
        # Ninja (clang-cl) does not allow the platform / architecture to be supplied from command line
        # It instead has to be supplied through environment flags, with the expected configuration set up,
        # typical of vcvars.
        set(X86CCLNinja            ${ThinBinaryDir}/CCLNinja86.bat)
        set(X86CommandBootstrapper ${X86CCLNinja} ${CMAKE_COMMAND})
        set(X86ArchFlags           -DTHIN_X86_M32=ON)

        # Create cmake bootstrapper
        configure_file(${CMAKE_SOURCE_DIR}/Build/Utils/CCLNinja86.bat.in ${X86CCLNinja})
    endif()
    
    # Configuration target
    # Note: I had great difficulty in getting the secondary-cmake targets to \DEPENDS on the sources, even
    #       when explicitly marked. So, a terrible workaround is to increment the V[X] below, and reconfigure.
    #       I'll find a proper solution at some point.
    add_custom_target(
        ThinX86Configure
        COMMAND ${CMAKE_COMMAND} -E echo Configuring X86-V103
        COMMAND ${X86CommandBootstrapper} -G "${CMAKE_GENERATOR}"
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DCMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
            -DCMAKE_C_COMPILER="${CMAKE_C_COMPILER}"
            -DTHIN_X86_BUILD=ON
            -DTHIN_X86_POSTFIX=X32
            -DENABLE_UIX=OFF
            ${X86ArchFlags}
            ${CMAKE_SOURCE_DIR}
        WORKING_DIRECTORY "${ThinBinaryDir}"
    )
    
    # Build target
    add_custom_target(
        ThinX86Build
        COMMAND ${CMAKE_COMMAND} -E echo Building X86-V103
        COMMAND ${X86CommandBootstrapper} --build . --config $<CONFIG>
        DEPENDS ThinX86Configure ${path}
        WORKING_DIRECTORY "${ThinBinaryDir}"
    )

    # Dependency helper
    set(ThinX86BuildDependency ThinX86Build)
endif()

# Ensure 64 bit
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT THIN_X86_BUILD)
    set(ARCH_POSTFIX "X64")
else()
    set(ARCH_POSTFIX "X32")
    
    # Force configuration for X86 build
    set(CMAKE_CONFIGURATION_TYPES Debug;Release;MinSizeRel;RelWithDebInfo)
    
    # Statically link runtimes
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")

    # Ensure the top build tree is x64
    if(NOT THIN_X86_BUILD)
        message(FATAL_ERROR "Only x64 is supported, x86 support limited to very specific modules")
    endif()
endif()

# Support up to 20
if (MSVC)
    # Clang(CL) frontend?
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL Clang)
        set(CMAKE_CXX_STANDARD 20)
    else()
        add_compile_options(/std:c++20 /Zc:__cplusplus)
    endif()
elseif(MINGW)
    set(CMAKE_CXX_STANDARD 20)

    # Sparsely maintained
    message(WARNING "MinGW support not complete")
else()
    message(FATAL_ERROR "Unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

# UIX bindings
if (${ENABLE_UIX} AND CMAKE_GENERATOR MATCHES "Visual Studio")
    set(BUILD_UIX ON)

    # Enable additional languages
    enable_language(CSharp)

    # Set standard C# properties
    SET(CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION "v4.8")
    SET(CMAKE_CSharp_FLAGS "/langversion:6")
    SET(CMAKE_CSharp_FLAGS "/platform:x64")
else()
    set(BUILD_UIX OFF)
endif()

# Ensure executables share the same runtimes
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD")
endif()

# Release configuration
if (${ENABLE_RELEASE_DEBUG})
    if (MSVC)
        # Clang(CL) frontend?
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL Clang)
            if (${CMAKE_BUILD_TYPE} MATCHES "Release")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Z7 /DEBUG")
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
            endif()
        else()
            set(MSVC_OPTIONS /Z7 /DEBUG)
            add_compile_options("$<$<CONFIG:Release>:${MSVC_OPTIONS}>")
        endif()
    else()
        if (${CMAKE_BUILD_TYPE} MATCHES "Release")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
        endif()
    endif()
endif()

# Set all outputs to Bin/Config
if (${ENABLE_MIXED_COMPILER})
    set(MixedCompilerPrefix "${CMAKE_CXX_COMPILER_ID}/")
endif()

# Single configurator paths
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Lib/${MixedCompilerPrefix}${CMAKE_BUILD_TYPE}${THIN_X86_POSTFIX})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Lib/${MixedCompilerPrefix}${CMAKE_BUILD_TYPE}${THIN_X86_POSTFIX})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${MixedCompilerPrefix}${CMAKE_BUILD_TYPE}${THIN_X86_POSTFIX})
    
# Multi-configurator paths
foreach(Config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${Config} ConfigUpper)
    
    # Libraries and archives are separated, share the binary outputs
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${ConfigUpper} ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${MixedCompilerPrefix}${Config})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${ConfigUpper} ${CMAKE_CURRENT_SOURCE_DIR}/Lib/${MixedCompilerPrefix}${Config}${THIN_X86_POSTFIX})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${ConfigUpper} ${CMAKE_CURRENT_SOURCE_DIR}/Lib/${MixedCompilerPrefix}${Config}${THIN_X86_POSTFIX})
endforeach()

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

# Add build support
if (NOT THIN_X86_BUILD)
    add_subdirectory(Build)
else()
    include(Build/Files.cmake)
endif()

# Add external projects
add_subdirectory(ThirdParty)

# Local compiler setup
if (MSVC)
    # Enable warnings are errors globally
    add_compile_options(/W3 /WX)

    # Clang(CL) frontend?
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL Clang)
		# Overly pedantic
        add_compile_options(/clang:-Wno-unused-private-field)
    endif()
elseif(MINGW)
    # Enable warnings are errors globally
	# TODO: -Wall is overly protective, and will break with new compiler versions
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
else()
    message(FATAL_ERROR "Unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

# Thin x86 build?
if (THIN_X86_BUILD)
    # Very few things need x86 support, it's less work to branch the exact libraries needed than introduce arch checking
    include(ThinX86.cmake)
else()
    # Full build
    add_subdirectory(Source)
endif()
