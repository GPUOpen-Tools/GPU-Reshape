
#----- Bridge -----#

add_library(
    Libraries.Bridge STATIC
    Source/MemoryBridge.cpp
)

# IDE source discovery
SetSourceDiscovery(Libraries.Bridge Include Source)

# Includes
target_include_directories(Libraries.Bridge PUBLIC Include ${CMAKE_CURRENT_BINARY_DIR}/Include)

# Links
target_link_libraries(Libraries.Bridge PUBLIC Libraries.Message)

#----- Tests -----#

# Create test layer
add_executable(
    Libraries.Bridge.Tests
    Tests/Source/Main.cpp
    Tests/Source/Emitter.cpp
)

# IDE source discovery
SetSourceDiscovery(Libraries.Bridge.Tests Tests)

# Includes
target_include_directories(Libraries.Bridge.Tests PUBLIC Tests/Include ${CMAKE_CURRENT_BINARY_DIR}/Tests/Include)

# Setup dependencies
ExternalProject_Link(Libraries.Bridge.Tests Catch2)

# Links
target_link_libraries(Libraries.Bridge.Tests PUBLIC Libraries.Bridge)
