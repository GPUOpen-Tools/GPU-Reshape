
# Add external library
include_external_msproject(UIX.Studio ${CMAKE_CURRENT_SOURCE_DIR}/Studio/Studio.csproj)

# IDE discovery
set_target_properties(UIX.Studio PROPERTIES FOLDER UIX/Studio)
