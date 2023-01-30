@echo off

rem Info
echo GPU Reshape - Solution generator
echo:
echo For the full set of configuration switches and options, see the build documentation.
echo:

rem Remove the old cache to avoid issues
rem if exist cmake-build-vs2019\ (
rem 	echo|set /p="Removing old cmake cache... "
rem 	rmdir /s /q "cmake-build-vs2019"
rem 	
rem 	rem Succeeded?
rem 	if %ERRORLEVEL% == 0 (
rem 		echo OK!
rem 	) else (
rem 		echo Failed!
rem 		exit /b 1
rem 	)
rem )

rem Print all generation inline in the command window
echo Generating from cmake...
echo ------------------------
echo:

cmake^
    -G "Visual Studio 16"^
    -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release;MinSizeRel;RelWithDebInfo"^
    -DINSTALL_THIRD_PARTY:BOOL="1"^
    -B "cmake-build-vs2019"^
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
cmake -P Build/Utils/CSProjPatch.cmake
echo OK!
