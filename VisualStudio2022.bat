:: 
:: The MIT License (MIT)
:: 
:: Copyright (c) 2023 Miguel Petersen
:: Copyright (c) 2023 Advanced Micro Devices, Inc
:: Copyright (c) 2023 Avalanche Studios Group
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy 
:: of this software and associated documentation files (the "Software"), to deal 
:: in the Software without restriction, including without limitation the rights 
:: to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
:: of the Software, and to permit persons to whom the Software is furnished to do so, 
:: subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in all 
:: copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
:: INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
:: PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
:: FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
:: ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
:: 

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
