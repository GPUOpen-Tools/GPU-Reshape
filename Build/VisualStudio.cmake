
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
