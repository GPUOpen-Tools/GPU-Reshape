
# Catch2, lightweight testing framework with micro-benchmarking support
ExternalProject_Add(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v2.13.7
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Catch2
    USES_TERMINAL_INSTALL 0
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCATCH_BUILD_STATIC_LIBRARY=ON
)
