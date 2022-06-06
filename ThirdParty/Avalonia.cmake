
# UI Framework
ExternalProject_Add(
    Avalonia
    GIT_REPOSITORY https://github.com/AvaloniaUI/Avalonia
    # GIT_TAG 0.10.14 -- Not tag locked yet
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Avalonia
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
