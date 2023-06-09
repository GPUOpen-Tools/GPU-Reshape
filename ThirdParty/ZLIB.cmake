
# Compression library
ExternalProject_Add(
    ZLIB
    GIT_REPOSITORY https://github.com/madler/zlib
    GIT_TAG master
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ZLIB
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -G ${CMAKE_GENERATOR}
)
