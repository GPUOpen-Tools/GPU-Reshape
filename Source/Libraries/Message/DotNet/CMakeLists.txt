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


# Create DotNet project
Project_AddDotNetEx(
    NAME GRS.Libraries.Message.DotNet
    LANG CS
    UNSAFE
	DEPENDENCIES
		GRS.Libraries.Message.DotNet.GenMerge
    SOURCE
        Source/MessageContainers.cs
        Source/MessageStream.cs
        Source/MessageSchema.cs
        Source/MessageSubStream.cs
        Source/ByteSpan.cs
		${GeneratedSchemaCS}
    ASSEMBLIES
        System
        System.Runtime
        System.Memory
)

# Benchmarking test
Project_AddDotNetEx(
    NAME GRS.Libraries.Message.Benchmark.DotNet
    LANG CS
    EXECUTABLE
    SOURCE
        Tests/Source/Benchmark.cs
    LIBS
        GRS.Libraries.Message.DotNet
    ASSEMBLIES
        System
        System.Runtime
        System.Memory
        System.Runtime.CompilerServices.Unsafe
)

# Quick message project
function(Project_AddSchemaDotNet NAME GEN_MERGE SCHEMA_GEN)
    Project_AddDotNetEx(
        NAME ${NAME}
        LANG CS
		DEPENDENCIES
			${GEN_MERGE}
		SOURCE
			${${SCHEMA_GEN}}
        ASSEMBLIES
            System
            System.Memory
        LIBS
            GRS.Libraries.Message.DotNet
    )
endfunction()
