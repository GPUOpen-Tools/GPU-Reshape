
taskkill /f /im Services.HostResolver.Standalone.exe 2>NUL | findstr SUCCESS >NUL
exit /b 0
