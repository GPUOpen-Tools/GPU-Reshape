
# Program argument parser
ExternalProject_Add(
    ArgParse
    GIT_REPOSITORY https://github.com/p-ranav/argparse
    GIT_TAG v2.1 # v2.2 contains invalid constexpr usage
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ArgParse
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -G ${CMAKE_GENERATOR}
)
