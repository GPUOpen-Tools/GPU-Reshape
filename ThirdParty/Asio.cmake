

# Program argument parser
ExternalProject_Add(
    Asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio
    GIT_TAG asio-1-21-0
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Asio
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Copy "${CMAKE_CURRENT_SOURCE_DIR}/Asio/asio/include" "${CMAKE_BINARY_DIR}/External/include" /s /d
)
