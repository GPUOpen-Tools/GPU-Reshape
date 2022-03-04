
function(Project_AddHLSL OUT_GENERATED PROFILE ARGS HLSL HEADER VAR)
    # Compiler path
    if (WIN32)
        set(CompilerPath ${CMAKE_SOURCE_DIR}/Tools/DXC/Win64/dxc.exe)
    else()
        set(CompilerPath ${CMAKE_SOURCE_DIR}/Tools/DXC/Linux/dxc)
    endif()

    # Hlsl path
    if (NOT IS_ABSOLUTE ${HLSL})
        set(Hlsl ${CMAKE_CURRENT_SOURCE_DIR}/${HLSL})
    else()
        set(Hlsl ${HLSL})
    endif()

    # Parse args
    separate_arguments(Args WINDOWS_COMMAND ${ARGS})

    # Generate the schema
    add_custom_command(
        OUTPUT ${HEADER}
        DEPENDS
            ${CompilerPath}
            ${Hlsl}
        COMMAND ${CompilerPath}
            -spirv
            -T${PROFILE}
            ${Hlsl}
            -Fh ${HEADER}
            -Vn ${VAR}
            ${Args}
    )

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${HEADER}" PARENT_SCOPE)
endfunction()
