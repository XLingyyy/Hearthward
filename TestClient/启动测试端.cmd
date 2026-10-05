@echo off
chcp 65001 >nul
cd /d "%~dp0.."
set "TEST_CLIENT_PYTHON=python"
if exist "E:\RealPython\python.exe" set "TEST_CLIENT_PYTHON=E:\RealPython\python.exe"
if exist "%~dp0..\..\.venv\Scripts\python.exe" set "TEST_CLIENT_PYTHON=%~dp0..\..\.venv\Scripts\python.exe"
if defined HEARTHWARD_PYTHON set "TEST_CLIENT_PYTHON=%HEARTHWARD_PYTHON%"
"%TEST_CLIENT_PYTHON%" -X utf8 "%~dp0..\scripts\ui\launch_test_client.py" %*
if errorlevel 1 if "%~1"=="" pause
