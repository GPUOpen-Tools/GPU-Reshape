
# D3D12MA, d3d12 memory management
ExternalProject_Add(
    D3D12MA
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator
    GIT_TAG v2.0.1
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/D3D12MA
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -G ${CMAKE_GENERATOR}
)
