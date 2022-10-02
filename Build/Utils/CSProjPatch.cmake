# Visual studio solution generation patchup
#   See https://gitlab.kitware.com/cmake/cmake/-/issues/23513

# Expected path
set(CSProjPath "cmake-build-vs2019/gpu-validation.sln")

# Replace all instances of "Any CPU" with "x64"
file(READ ${CSProjPath} SolutionContents)
string(REPLACE "Any CPU" "x64" FILE_CONTENTS "${SolutionContents}")
file(WRITE ${CSProjPath} ${FILE_CONTENTS})
