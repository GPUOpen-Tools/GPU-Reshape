
#----- Backend -----#

add_library(
    Libraries.Backend STATIC
    Source/Backend.cpp
)

target_include_directories(Libraries.Backend PUBLIC Include)

#----- Tests -----#

# Create test layer
add_executable(
    Libraries.Backend.Tests
    Tests/Source/Main.cpp
    Tests/Source/Emitter.cpp
    Tests/Source/Feature.cpp
)

# Setup dependencies
ExternalProject_Link(Libraries.Backend.Tests Catch2)

# Links
target_link_libraries(Libraries.Backend.Tests Libraries.Backend)
