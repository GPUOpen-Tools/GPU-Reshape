
# Fmt, C++20 compliant formatting
ExternalProject_Add(
    Fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 8.1.1
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Fmt
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -G ${CMAKE_GENERATOR}
)
