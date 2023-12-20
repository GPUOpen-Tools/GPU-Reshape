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

# Quick library setup
function(Project_AddDotNet NAME)
    add_library(
        ${NAME} SHARED
        ${ARGN}
    )

    # DotNet
    set_target_properties(
        ${NAME} PROPERTIES
        VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.8"
        VS_GLOBAL_ROOTNAMESPACE ${NAME}
    )

    # IDE source discovery
    SetSourceDiscovery(${NAME} CS Include Source Schema)
endfunction()

function(Project_AddDotNetEx)
    cmake_parse_arguments(
        ARGS
        "UNSAFE;EXECUTABLE" # Options
        "NAME;LANG;PROPS" # One Value
        "SOURCE;GENERATED;ASSEMBLIES;LIBS;FLAGS" # Multi Value
        ${ARGN}
    )

    if (NOT "${ARGS_GENERATED}" STREQUAL "")
        # Generate sham target
        #   ! WORKAROUND, Visual Studio generators do not support C# sources from add_custom_command
        #                 Check introduced by 3.24
        add_library(${ARGS_NAME}.Sham INTERFACE ${${ARGS_GENERATED}_Sham})
		
		# Create dummy file to keep MSVC happy
		if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${${ARGS_GENERATED}_Sham}")
			file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${${ARGS_GENERATED}_Sham}" Sham)
		endif()
    endif()

    # Create library
    if ("${ARGS_EXECUTABLE}")
        add_executable(
            ${ARGS_NAME}
            ${ARGS_SOURCE}
            ${${ARGS_GENERATED}}
        )
    else()
        add_library(
            ${ARGS_NAME} SHARED
            ${ARGS_SOURCE}
            ${${ARGS_GENERATED}}
        )
    endif()

    # CLI Includes
    if ("${ARGS_LANG}" STREQUAL "CXX")
        target_include_directories(${ARGS_NAME} PUBLIC Include)

        # Enable CLR
        set_target_properties(${ARGS_NAME} PROPERTIES COMMON_LANGUAGE_RUNTIME "")
        
		# C++/CLI does not support 20 at the moment, downgrade to 17 
		get_target_property(CLIOptions ${ARGS_NAME} COMPILE_OPTIONS)
		list(REMOVE_ITEM CLIOptions "/std:c++20")
		list(APPEND CLIOptions "/std:c++17")
		set_property(TARGET ${ARGS_NAME} PROPERTY COMPILE_OPTIONS ${CLIOptions})
    endif()

    # Set .NET, link to assemblies and libs
    set_target_properties(
        ${ARGS_NAME} PROPERTIES
        VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.8"
        VS_GLOBAL_ROOTNAMESPACE "${ARGS_NAME}"
        VS_DOTNET_REFERENCES "${ARGS_ASSEMBLIES};${ARGS_LIBS}"
        VS_USER_PROPS "${CMAKE_SOURCE_DIR}/Build/cs.configuration.props"
    )

    # Add flags
    if (NOT "${ARGS_FLAGS}" STREQUAL "")
        target_compile_options(${ARGS_NAME} PUBLIC "${ARGS_FLAGS}")
    endif()

    # Unsafe compilation?
    if ("${ARGS_UNSAFE}")
        set_target_properties(${ARGS_NAME} PROPERTIES VS_GLOBAL_AllowUnsafeBlocks "true")
    endif()

    # Set properties
    if (NOT "${ARGS_PROPS}" STREQUAL "")
        set_target_properties(
            ${ARGS_NAME} PROPERTIES
            VS_USER_PROPS "${CMAKE_CURRENT_SOURCE_DIR}/${ARGS_PROPS}"
        )
    endif()

    # Add explicit links and dependencies
    if (NOT "${ARGS_LIBS}" STREQUAL "")
        target_link_libraries(${ARGS_NAME} PUBLIC ${ARGS_LIBS})

        if (NOT "${ARGS_LANG}" STREQUAL "CXX")
            add_dependencies(${ARGS_NAME} ${ARGS_LIBS})
        endif()
    endif()

    # Reference sham target to let dependencies generate before use
    if (NOT "${ARGS_GENERATED}" STREQUAL "")
        add_dependencies(${ARGS_NAME} ${ARGS_NAME}.Sham)
    endif()

    # IDE source discovery
    SetSourceDiscovery(${ARGS_NAME} ${ARGS_LANG} Include Source)
endfunction()
