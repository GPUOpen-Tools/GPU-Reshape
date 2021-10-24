
# TinyXML, fast XML reader
ExternalProject_Add(
    TinyXML2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2
    GIT_TAG 9.0.0
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/TinyXML2
    USES_TERMINAL_INSTALL 0
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/External
)
