@echo off
rem Build and run PluginHostName unit tests:
rem   1. HostNameFile core logic standalone test
rem   2. Plugin DLL load integration test (requires Release x64 dll built)
rem Usage: cmd /c <path-to-this-file>
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed
    exit /b 1
)
cd /d "%~dp0.."

rem --- test 1: HostNameFile core logic ---
if exist tests\test_hostname_file.exe del tests\test_hostname_file.exe
cl /nologo /EHsc /utf-8 tests\test_hostname_file.cpp HostNameFile.cpp /Fe:tests\test_hostname_file.exe
if errorlevel 1 (
    echo [ERROR] test 1 compile failed
    exit /b 1
)
tests\test_hostname_file.exe
if errorlevel 1 exit /b 1

rem --- test 2: plugin dll load integration (needs ..\Bin\x64\Release\plugins\PluginHostName.dll) ---
if not exist "..\Bin\x64\Release\plugins\PluginHostName.dll" (
    echo [SKIP] dll not found, build it first: MSBuild TrafficMonitor.sln -t:PluginHostName -p:Configuration=Release -p:Platform=x64
    exit /b 0
)
if exist tests\test_plugin_load.exe del tests\test_plugin_load.exe
cl /nologo /EHsc /utf-8 /I "..\..\include" tests\test_plugin_load.cpp /Fe:tests\test_plugin_load.exe
if errorlevel 1 (
    echo [ERROR] test 2 compile failed
    exit /b 1
)
tests\test_plugin_load.exe "..\Bin\x64\Release\plugins\PluginHostName.dll"
exit /b %errorlevel%
