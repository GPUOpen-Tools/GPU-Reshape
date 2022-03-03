
function(Project_AddBackendTest_Common OUT_GENERATED FEATURE_NAME)
    set(DataDir ${CMAKE_SOURCE_DIR}/Source/Features/Common/Backend/Tests/Data)

    # Add tests
    Project_AddBackendTest(Generated ${FEATURE_NAME} ${DataDir}/ControlFlowSelectionMerge.hlsl)
    Project_AddBackendTest(Generated ${FEATURE_NAME} ${DataDir}/ControlFlowLoopMerge.hlsl)
    Project_AddBackendTest(Generated ${FEATURE_NAME} ${DataDir}/ControlFlowSwitch.hlsl)

    # Set output
    set(${OUT_GENERATED} "${${OUT_GENERATED}};${Generated}" PARENT_SCOPE)
endfunction()
