@echo off

rmdir "cmake-build-vs2019"

cmake^
    -G "Visual Studio 16"^
    -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release;MinSizeRel;RelWithDebInfo"^
    -DINSTALL_THIRD_PARTY:BOOL="1"^
    -B "cmake-build-vs2019"
