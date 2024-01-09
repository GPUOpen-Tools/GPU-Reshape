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


function(Project_AddBackendTest_Common OUT_GENERATED FEATURE_NAME)
    set(DataDir ${CMAKE_SOURCE_DIR}/Source/Features/Common/Backend/Tests/Data)

    # Add tests
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/ControlFlowSelectionMerge.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/ControlFlowLoopMerge.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/ControlFlowSwitch.hlsl)
    Project_AddBackendTest(Generated "-O3" ${FEATURE_NAME} ${DataDir}/Phi.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/Bindings.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/Empty.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/MultiFile.hlsl)
    Project_AddBackendTest(Generated "-Od -rootsig-define RS" ${FEATURE_NAME} ${DataDir}/EmbeddedRootSignature.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/Intrinsics.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/Structural.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/GroupShared.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/ExternalPDB.hlsl D3D12 "-Zi -Qstrip_debug -Fd ExternalPDB.pdb")

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${Generated}" PARENT_SCOPE)
endfunction()
