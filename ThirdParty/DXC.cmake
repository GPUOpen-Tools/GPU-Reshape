
function(Project_AddHLSL OUT_GENERATED PROFILE ARGS HLSL HEADER VAR)
    # Compiler path
    if (WIN32)
        set(CompilerPath ${CMAKE_SOURCE_DIR}/ThirdParty/DXC/bin/Win64/dxc.exe)
    else()
        set(CompilerPath ${CMAKE_SOURCE_DIR}/ThirdParty/DXC/bin/Linux/dxc)
    endif()

    # Hlsl path
    if (NOT IS_ABSOLUTE ${HLSL})
        set(Hlsl ${CMAKE_CURRENT_SOURCE_DIR}/${HLSL})
    else()
        set(Hlsl ${HLSL})
    endif()

    # Parse args
    separate_arguments(Args WINDOWS_COMMAND ${ARGS})

    # Generate vulkan
    if (${ENABLE_BACKEND_VULKAN})
        add_custom_command(
            OUTPUT ${HEADER}Vulkan.h
            DEPENDS
                ${CompilerPath}
                ${Hlsl}
            COMMAND ${CompilerPath}
                -spirv
                -Zi
                -Qembed_debug
                -T${PROFILE}
                ${Hlsl}
                -Fh ${HEADER}Vulkan.h
                -Vn ${VAR}Vulkan
                ${Args}
        )
    endif()

    # Generate d3d12
    if (${ENABLE_BACKEND_DX12})
        add_custom_command(
            OUTPUT ${HEADER}D3D12.h
            DEPENDS
                ${CompilerPath}
                ${Hlsl}
            COMMAND ${CompilerPath}
                -T${PROFILE}
                -Zi
                -Qembed_debug
                ${Hlsl}
                -Fh ${HEADER}D3D12.h
                -Vn ${VAR}D3D12
                ${Args}
        )
    endif()

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${HEADER}Vulkan.h;${HEADER}D3D12.h" PARENT_SCOPE)
endfunction()
