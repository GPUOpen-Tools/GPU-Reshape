
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

    # Default arguments
    set(SupressArguments -Wno-unknown-attributes -Wno-ignored-attributes)

    # Backend specific arguments
    cmake_parse_arguments(
        ARGS
        "" # Options
        "D3D12;VULKAN" # One Value
        "" # Multi Value
        ${ARGN}
    )

    # D3D12 defaults
    if ("${ARGS_D3D12}" STREQUAL "")
        set(ARGS_D3D12 "-Zi -Qembed_debug")
    endif ()

    # Vulkan defaults
    if ("${ARGS_VULKAN}" STREQUAL "")
        set(ARGS_VULKAN "-Zi -Qembed_debug")
    endif ()

    # Parse additional args
    separate_arguments(ArgsD3D12 WINDOWS_COMMAND ${ARGS_D3D12})
    separate_arguments(ArgsVulkan WINDOWS_COMMAND ${ARGS_VULKAN})

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
                ${SupressArguments}
                ${Args} ${ArgsVulkan}
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
                ${Hlsl}
                -Fh ${HEADER}D3D12.h
                -Vn ${VAR}D3D12
                ${SupressArguments}
                ${Args} ${ArgsD3D12}
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
                    ${Hlsl}
                    -Fc ${HEADER}D3D12.dxil.txt
                    ${SupressArguments}
                    ${Args} ${ArgsD3D12}
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
