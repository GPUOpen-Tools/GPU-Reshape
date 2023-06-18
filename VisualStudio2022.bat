@echo off

rem Info
echo GPU Reshape - Solution generator
echo:
echo For the full set of configuration switches and options, see the build documentation.
echo:

rem Print all generation inline in the command window
echo Generating from cmake...
echo ------------------------
echo:

cmake^
    -G "Visual Studio 17"^
    -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release;MinSizeRel;RelWithDebInfo"^
    -DINSTALL_THIRD_PARTY:BOOL="1"^
    -B "cmake-build-vs2022"^
    %*
	
echo:
echo -------------------------

rem Succeeded?
if %ERRORLEVEL% == 0 (
	echo OK!
	echo:
) else (
	echo Failed!
	exit /b 1
)

echo|set /p="Solution patching... "
cmake -P Build/Utils/CSProjPatch.cmake cmake-build-vs2022
echo OK!
