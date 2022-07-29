
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

    # Generated dependencies
    set(InternalGenerated "")

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

        # Add output
        list(APPEND InternalGenerated ${HEADER}Vulkan.h)
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

        # Add output
        list(APPEND InternalGenerated ${HEADER}D3D12.h)

        if (${ENABLE_DXIL_DUMP})
            add_custom_command(
                OUTPUT ${HEADER}D3D12.dxil.txt
                DEPENDS
                    ${CompilerPath}
                    ${Hlsl}
                COMMAND ${CompilerPath}
                    -T${PROFILE}
                    -Zi
                    -Qembed_debug
                    ${Hlsl}
                    -Fc ${HEADER}D3D12.dxil.txt
                    ${Args}
            )

            # Add output
            list(APPEND InternalGenerated ${HEADER}D3D12.dxil.txt)
        endif()
    endif()

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${InternalGenerated}" PARENT_SCOPE)
endfunction()

# Get all includes
file(GLOB Sources DXC/include/*)

# Configure all includes
foreach (File ${Sources})
    get_filename_component(BaseName "${File}" NAME)
    configure_file(${File} ${CMAKE_BINARY_DIR}/External/include/DXC/${BaseName})
endforeach ()

# Copy binaries
if (WIN32)
    ConfigureOutput(DXC/bin/win64/dxil.dll dxil.dll)
    ConfigureOutput(DXC/bin/win64/dxcompiler.dll dxcompiler.dll)
endif()
