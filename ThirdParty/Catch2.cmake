
# Catch2, lightweight testing framework with micro-benchmarking support
ExternalProject_Add(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v2.13.7
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Catch2
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCATCH_BUILD_STATIC_LIBRARY=ON
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -DCATCH_BUILD_TESTING=OFF
        -G ${CMAKE_GENERATOR}
)
