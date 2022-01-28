
# https://stackoverflow.com/questions/31422680/how-to-set-visual-studio-filters-for-nested-sub-directory-using-cmake
function(VisualStudioSourceStructure SourceDir)
    foreach(Source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${Source}")
            file(RELATIVE_PATH Relative "${SourceDir}" "${Source}")
        else()
            set(Relative "${Source}")
        endif()
        get_filename_component(SourcePath "${Relative}" PATH)
        string(REPLACE "/" "\\" SourceMSVC "${SourcePath}")
        source_group("${SourceMSVC}" FILES "${Source}")
    endforeach()
endfunction(VisualStudioSourceStructure)

function(VisualStudioProjetPostfix NAME)
    # Get properties
    get_target_property(SourceDir ${NAME} SOURCE_DIR)
    get_target_property(SourceList ${NAME} SOURCES)

    # Apply directory structure to given source files
    VisualStudioSourceStructure(${SourceDir} ${SourceList})

    # Recreate the file tree from the first source
    # This is not entirely accurate, but will suffice for now
    list(GET SourceList 0 FirstSource)
    if ("${FirstSource}" STREQUAL "SourceList-NOTFOUND")
        return()
    endif()

    # Base directory
    string(FIND "${FirstSource}" "/" DirectoryEnd)

    # Discover common dependencies
    if (NOT ${DirectoryEnd} STREQUAL "-1")
        # Discover base directory
        string(SUBSTRING "${FirstSource}" 0 ${DirectoryEnd} Directory)
        file(GLOB_RECURSE BaseDirSourceTree ${Directory}/*.*)

        # Exclude sources files, added from user given sources
        list(FILTER BaseDirSourceTree EXCLUDE REGEX ".cpp")
        if (NOT "${BaseDirSourceTree}" STREQUAL "")
            # Add as private sources
            target_sources(${NAME} PRIVATE ${BaseDirSourceTree})

            # Mark added sources as header only, preventing build rules from kicking in
            set_source_files_properties(${BaseDirSourceTree} PROPERTIES HEADER_FILE_ONLY TRUE)

            # Apply directory structure to source files
            VisualStudioSourceStructure(${SourceDir} ${BaseDirSourceTree})
        endif()
    endif()

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

function(add_library NAME)
    _add_library(${NAME} ${ARGN})
    VisualStudioProjetPostfix(${NAME})
endfunction(add_library)

function(add_executable NAME)
    _add_executable(${NAME} ${ARGN})
    VisualStudioProjetPostfix(${NAME})
endfunction(add_executable)
