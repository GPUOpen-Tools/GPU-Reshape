# 
# The MIT License (MIT)
# 
# Copyright (c) 2023 Advanced Micro Devices, Inc.,
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


function(VisualStudioSourceStructure SourceDir BinaryDir)
    # Partially based on
    # https://stackoverflow.com/questions/31422680/how-to-set-visual-studio-filters-for-nested-sub-directory-using-cmake
    foreach(Source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${Source}")
            file(RELATIVE_PATH Relative "${SourceDir}" "${Source}")
        else()
            set(Relative "${Source}")
        endif()
        get_filename_component(SourcePath "${Relative}" PATH)
        string(REPLACE "/" "\\" SourceMSVC "${SourcePath}")

        if (EXISTS "${SourceDir}/${Relative}")
            source_group("${SourceMSVC}" FILES "${Source}")
        else()
            source_group("${SourceMSVC}" FILES "${BinaryDir}/${Source}")
        endif()
    endforeach()
endfunction(VisualStudioSourceStructure)

function(VisualStudioProjectPostfix NAME)
    # Skip X86 proxy targets
    if (${NAME} MATCHES "ThinX86.*")
        return()
    endif()

    # Get properties
    get_target_property(SourceDir ${NAME} SOURCE_DIR)
    get_target_property(BinaryDir ${NAME} BINARY_DIR)
    get_target_property(SourceList ${NAME} SOURCES)

    # Apply directory structure to given source files
    VisualStudioSourceStructure(${SourceDir} ${BinaryDir} ${SourceList})

    # Folder structure and label
    if ("${RELATIVE_PATH}" MATCHES "ThirdParty")
        set_target_properties(Backends.Vulkan.Generator PROPERTIES FOLDER ThirdParty/${NAME})
    else()
        string(FIND "${NAME}" "." NameBegin REVERSE)
        if (NOT ${NameBegin} STREQUAL "-1")
            MATH(EXPR NameBegin "${NameBegin}+1")
            string(SUBSTRING "${NAME}" ${NameBegin} -1 Label)
        else()
            set(Label "${NAME}")
        endif()

        # Project label
        set_target_properties(${NAME} PROPERTIES PROJECT_LABEL ${Label})

        # Project directory
        file(RELATIVE_PATH RelativeSource "${CMAKE_SOURCE_DIR}" "${SourceDir}")
        set_target_properties(${NAME} PROPERTIES FOLDER ${RelativeSource})
    endif()
endfunction()

function(SetVisualStudioSourceDiscovery NAME LANG)
    # Get properties
    get_target_property(SourceDir ${NAME} SOURCE_DIR)
    get_target_property(BinaryDir ${NAME} BINARY_DIR)

    foreach (Directory IN ITEMS ${ARGN})
        # Discover base directory
        file(GLOB_RECURSE BaseDirSourceTree ${Directory}/*.*)

        # Exclude sources files, added from user given sources
        if (LANG STREQUAL "CXX")
            list(FILTER BaseDirSourceTree EXCLUDE REGEX ".cpp|.cs")
        elseif(LANG STREQUAL "CS")
            # All relevant sources added already
            set(BaseDirSourceTree "")
        else()
            message(FATAL_ERROR "Invalid language: ${LANG}")
        endif()

        if (NOT "${BaseDirSourceTree}" STREQUAL "")
            # Add as private sources
            target_sources(${NAME} PRIVATE ${BaseDirSourceTree})

            # Mark added sources as header only, preventing build rules from kicking in
            set_source_files_properties(${BaseDirSourceTree} PROPERTIES HEADER_FILE_ONLY TRUE)

            # Apply directory structure to source files
            VisualStudioSourceStructure(${SourceDir} ${BinaryDir} ${BaseDirSourceTree})
        endif()
    endforeach()
endfunction()

function(add_library NAME)
    _add_library(${NAME} ${ARGN})
    VisualStudioProjectPostfix(${NAME})
endfunction(add_library)

function(add_executable NAME)
    _add_executable(${NAME} ${ARGN})
    VisualStudioProjectPostfix(${NAME})
endfunction(add_executable)

function(add_custom_target NAME)
    _add_custom_target(${NAME} ${ARGN})
    VisualStudioProjectPostfix(${NAME})
endfunction(add_custom_target)

function(include_external_msproject NAME PATH)
    _include_external_msproject("${NAME}" "${PATH}")

    # Force restore of all available packages
    if (${ENABLE_NUGET_RESTORE})
        message("Restoring nuget packages in ${NAME}")
        execute_process(
            COMMAND ${CMAKE_SOURCE_DIR}/ThirdParty/Nuget/nuget.exe restore "${PATH}"
            RESULT_VARIABLE Result
            OUTPUT_VARIABLE Output
            ERROR_VARIABLE  Output
        )

        # Failed?
        if (NOT "${Result}" STREQUAL "0")
            message(FATAL_ERROR "Failed to restore nuget packages in ${PATH}, output:\n${Output}")
        endif()
    endif()
endfunction(include_external_msproject)
