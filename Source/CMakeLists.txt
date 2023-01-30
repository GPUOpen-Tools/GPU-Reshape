
# Base set of libraries
add_subdirectory(Libraries)

# Testing suite
add_subdirectory(Test)

# Common implementations / variants of libraries
add_subdirectory(Backends)

# All user features
add_subdirectory(Features)

# Standalone applications
add_subdirectory(Services)

# UIX
if (${BUILD_UIX})
    add_subdirectory(UIX)
endif ()
