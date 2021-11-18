
function(Project_AddHLSL OUT_GENERATED PROFILE HLSL HEADER VAR)
    # Compiler path
    if (WIN32)
        set(CompilerPath ${CMAKE_SOURCE_DIR}/Tools/DXC/Win64/dxc.exe)
    else()
        set(CompilerPath ${CMAKE_SOURCE_DIR}/Tools/DXC/Linux/dxc)
    endif()

    # Hlsl path
    set(Hlsl ${CMAKE_CURRENT_SOURCE_DIR}/${HLSL})

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
    )

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${HEADER}" PARENT_SCOPE)
endfunction()
