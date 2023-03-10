
# Lightweight b'tree implementation
ExternalProject_Add(
    BTree
    GIT_REPOSITORY https://github.com/frozenca/BTree
    GIT_TAG main
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/BTree
    USES_TERMINAL_INSTALL 0
    UPDATE_DISCONNECTED ${ThirdPartyDisconnected}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -DBTREE_PATH=${CMAKE_SOURCE_DIR}/ThirdParty/BTree -P "${CMAKE_SOURCE_DIR}/Build/Utils/BTreeClangPatch.cmake"
    BUILD_COMMAND ""
    INSTALL_COMMAND COMMAND ${CMAKE_SOURCE_DIR}/Build/Utils/Copy "${CMAKE_CURRENT_SOURCE_DIR}/BTree" "${CMAKE_BINARY_DIR}/External/include/btree" /s /d
)
