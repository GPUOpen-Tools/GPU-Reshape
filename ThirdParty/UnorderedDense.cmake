
# Excellent hash map implementation
ExternalProject_Add(
    UnorderedDense
    GIT_REPOSITORY https://github.com/martinus/unordered_dense
    GIT_TAG v3.1.1
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/UnorderedDense
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND COMMAND ${CMAKE_SOURCE_DIR}/Build/Utils/Copy "${CMAKE_CURRENT_SOURCE_DIR}/UnorderedDense/include" "${CMAKE_BINARY_DIR}/External/include" /s /d
)
