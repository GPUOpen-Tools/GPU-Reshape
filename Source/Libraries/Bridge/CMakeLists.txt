
#----- Bridge -----#

# Generate the schemas
Project_AddSchema(GeneratedLib Schemas/PingPong.xml Include/Schemas)

add_library(
    Libraries.Bridge STATIC
    Source/MemoryBridge.cpp
    Source/NetworkBridge.cpp
    Source/Network/PingPongListener.cpp
    Source/Log/LogConsoleListener.cpp
    Source/Log/LogBuffer.cpp

    # Generated
    ${GeneratedLib}
)

# IDE source discovery
SetSourceDiscovery(Libraries.Bridge Include Source)

# Includes
target_include_directories(Libraries.Bridge PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Setup dependencies
ExternalProject_Link(Libraries.Bridge Asio)
ExternalProject_Link(Libraries.Bridge Fmt $<$<CONFIG:Debug>:fmtd> $<$<CONFIG:Release>:fmt>)

# Links
target_link_libraries(Libraries.Bridge PUBLIC Libraries.Message)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(Libraries.Bridge PRIVATE /EHa)
endif()

#----- Tests -----#

# Create test layer
add_executable(
    Libraries.Bridge.Tests
    Tests/Source/Main.cpp
    Tests/Source/Emitter.cpp
    Tests/Source/Network.cpp
)

# IDE source discovery
SetSourceDiscovery(Libraries.Bridge.Tests Tests)

# Includes
target_include_directories(Libraries.Bridge.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(Libraries.Bridge.Tests Catch2)

# Links
target_link_libraries(Libraries.Bridge.Tests PUBLIC Libraries.Bridge)
