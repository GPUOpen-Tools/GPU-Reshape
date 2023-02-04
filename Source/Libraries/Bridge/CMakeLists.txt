
#----- Bridge -----#

# Generate the schemas
Project_AddSchema(GeneratedLibSchema Schemas/PingPong.xml Include/Schemas)
Project_AddSchema(GeneratedLibSchema Schemas/HostResolve.xml Include/Schemas)

add_library(
    GRS.Libraries.Bridge STATIC
    Source/MemoryBridge.cpp
    Source/HostServerBridge.cpp
    Source/RemoteClientBridge.cpp
    Source/Network/PingPongListener.cpp
    Source/Log/LogConsoleListener.cpp
    Source/Log/LogBuffer.cpp
    Source/Asio/AsioDebug.cpp

    # Generated
    ${GeneratedLibSchemaCPP}
)

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Bridge CXX Include Source)

# Includes
target_include_directories(GRS.Libraries.Bridge PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Setup dependencies
ExternalProject_Link(GRS.Libraries.Bridge Asio)

# Links
target_link_libraries(GRS.Libraries.Bridge PUBLIC GRS.Libraries.Message)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Libraries.Bridge PRIVATE /EHa)
endif()

#----- Tests -----#

# Create test layer
add_executable(
    GRS.Libraries.Bridge.Tests
    Tests/Source/Main.cpp
    Tests/Source/Emitter.cpp
    Tests/Source/Asio.cpp
)

# Enable exceptions, only for clang-cl based compilers which seem to have it disabled implicitly
if (MSVC)
    target_compile_options(GRS.Libraries.Bridge.Tests PRIVATE /EHa)
endif()

# IDE source discovery
SetSourceDiscovery(GRS.Libraries.Bridge.Tests CXX Tests)

# Includes
target_include_directories(GRS.Libraries.Bridge.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(GRS.Libraries.Bridge.Tests Catch2)

# Links
target_link_libraries(GRS.Libraries.Bridge.Tests PUBLIC GRS.Libraries.Bridge)

#----- .Net bindings -----#

if (${BUILD_UIX})
    include(DotNet.cmake)
endif()
