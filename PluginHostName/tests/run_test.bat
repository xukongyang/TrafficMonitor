@echo off
rem Build and run HostNameFile unit tests (standalone test program).
rem Usage: cmd /c <path-to-this-file>
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed
    exit /b 1
)
cd /d "%~dp0.."
if exist tests\test_hostname_file.exe del tests\test_hostname_file.exe
cl /nologo /EHsc /utf-8 tests\test_hostname_file.cpp HostNameFile.cpp /Fe:tests\test_hostname_file.exe
if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)
tests\test_hostname_file.exe
exit /b %errorlevel%
