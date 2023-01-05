
#----- Unified tests -----#

add_executable(
    Test.Unified
    Source/main.cpp
)

function(Project_AddUnifiedTestLibrary NAME)
    target_link_libraries(Test.Unified PRIVATE ${NAME})
endfunction()

function(Project_AddTest)
    cmake_parse_arguments(
        ARGS
        "" # Options
        "NAME" # One Value
        "SOURCE;LIBS" # Multi Value
        ${ARGN}
    )

    # Get properties
    get_target_property(SourceDir Test.Unified SOURCE_DIR)

    # Create test library
    add_library(
        ${ARGS_NAME}.Lib OBJECT
        ${ARGS_SOURCE}
    )

    if(NOT "${ARGS_LIBS}" STREQUAL "")
        target_link_libraries(${ARGS_NAME}.Lib PUBLIC ${ARGS_LIBS})
    endif()

    # Create test app
    add_executable(
        ${ARGS_NAME}
        ${SourceDir}/Source/main.cpp
    )

    target_link_libraries(${ARGS_NAME} PUBLIC ${ARGS_NAME}.Lib)

    Project_AddUnifiedTestLibrary(${ARGS_NAME}.Lib)

    # IDE source discovery
    SetSourceDiscovery(${ARGS_NAME}.Lib CXX Tests)
    SetSourceDiscovery(${ARGS_NAME} CXX Tests)
endfunction()
