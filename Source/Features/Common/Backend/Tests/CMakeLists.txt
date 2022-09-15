
function(Project_AddBackendTest_Common OUT_GENERATED FEATURE_NAME)
    set(DataDir ${CMAKE_SOURCE_DIR}/Source/Features/Common/Backend/Tests/Data)

    # Add tests
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/ControlFlowSelectionMerge.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/ControlFlowLoopMerge.hlsl)
    Project_AddBackendTest(Generated "-Od" ${FEATURE_NAME} ${DataDir}/ControlFlowSwitch.hlsl)
    Project_AddBackendTest(Generated "-O3" ${FEATURE_NAME} ${DataDir}/Phi.hlsl)
    Project_AddBackendTest(Generated "-O3" ${FEATURE_NAME} ${DataDir}/Bindings.hlsl)
    Project_AddBackendTest(Generated "-O3" ${FEATURE_NAME} ${DataDir}/Empty.hlsl)
    Project_AddBackendTest(Generated "-O3" ${FEATURE_NAME} ${DataDir}/MultiFile.hlsl)
    Project_AddBackendTest(Generated "-O3 -rootsig-define RS" ${FEATURE_NAME} ${DataDir}/EmbeddedRootSignature.hlsl)

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${Generated}" PARENT_SCOPE)
endfunction()
