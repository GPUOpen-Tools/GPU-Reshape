rem 
rem The MIT License (MIT)
rem 
rem Copyright (c) 2023 Miguel Petersen
rem Copyright (c) 2023 Advanced Micro Devices, Inc
rem Copyright (c) 2023 Avalanche Studios Group
rem 
rem Permission is hereby granted, free of charge, to any person obtaining a copy 
rem of this software and associated documentation files (the "Software"), to deal 
rem in the Software without restriction, including without limitation the rights 
rem to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
rem of the Software, and to permit persons to whom the Software is furnished to do so, 
rem subject to the following conditions:
rem 
rem The above copyright notice and this permission notice shall be included in all 
rem copies or substantial portions of the Software.
rem 
rem THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
rem INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
rem PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
rem FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
rem ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
rem 

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
