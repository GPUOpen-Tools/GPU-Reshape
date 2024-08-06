# 
# The MIT License (MIT)
# 
# Copyright (c) 2024 Advanced Micro Devices, Inc.,
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
		DOTNET_SDK "Microsoft.NET.Sdk"
		DOTNET_TARGET_FRAMEWORK "net8.0"
        VS_GLOBAL_Platforms "x64"
		VS_GLOBAL_RuntimeIdentifier "win-x64"
		VS_GLOBAL_AppendTargetFrameworkToOutputPath "false"
		VS_GLOBAL_AppendRuntimeIdentifierToOutputPath "false"
        VS_GLOBAL_ROOTNAMESPACE ${NAME}
    )

    # IDE source discovery
    SetSourceDiscovery(${NAME} CS Include Source Schema)
endfunction()

# Add a .NET merge target for generated files
#  ! Must be called in the same CMakeLists.txt as the custom commands
function(Project_AddDotNetGeneratedMerge NAME GENERATED_SOURCES)
	set(Command "")
	
	# Copy each schema target
	foreach(File ${${GENERATED_SOURCES}})
		list(APPEND Command COMMAND "${CMAKE_COMMAND}" -E copy "${File}.gen" "${File}")
		
		# Generated project / MSBUILD does not check that if inbound file originates from
		# another target.
		if (NOT EXISTS ${File})
			file(WRITE "${File}" "Generation target file")
		endif()
	endforeach()
	
	# Schema Gen -> Schema
	add_custom_target(
		${NAME}
		DEPENDS ${${GENERATED_SOURCES}_Gen}
		BYPRODUCTS ${${GENERATED_SOURCES}}
		${Command}
	)
endfunction()

function(Project_AddDotNetEx)
    cmake_parse_arguments(
        ARGS
        "UNSAFE;EXECUTABLE" # Options
        "NAME;LANG;PROPS" # One Value
        "SOURCE;DEPENDENCIES;ASSEMBLIES;LIBS;FLAGS" # Multi Value
        ${ARGN}
    )

    # Create library
    if ("${ARGS_EXECUTABLE}")
        add_executable(
            ${ARGS_NAME}
            ${ARGS_SOURCE}
        )
    else()
        add_library(
            ${ARGS_NAME} SHARED
            ${ARGS_SOURCE}
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
		DOTNET_SDK "Microsoft.NET.Sdk"
		DOTNET_TARGET_FRAMEWORK "net8.0"
        VS_GLOBAL_Platforms "x64"
		VS_GLOBAL_RuntimeIdentifier "win-x64"
		VS_GLOBAL_AppendTargetFrameworkToOutputPath "false"
		VS_GLOBAL_AppendRuntimeIdentifierToOutputPath "false"
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

    # Add additional dependencies	
	if (NOT "${ARGS_DEPENDENCIES}" STREQUAL "")
        add_dependencies(${ARGS_NAME} ${ARGS_DEPENDENCIES})
	endif()

    # IDE source discovery
    SetSourceDiscovery(${ARGS_NAME} ${ARGS_LANG} Include Source)
endfunction()
