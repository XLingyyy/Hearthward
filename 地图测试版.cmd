@echo off
chcp 65001 >nul
cd /d "%~dp0"
set "MAP_TEST_PYTHON=python"
if exist "E:\RealPython\python.exe" set "MAP_TEST_PYTHON=E:\RealPython\python.exe"
if defined HEARTHWARD_PYTHON set "MAP_TEST_PYTHON=%HEARTHWARD_PYTHON%"
"%MAP_TEST_PYTHON%" -X utf8 "scripts\ui\launch_map_test.py"
if errorlevel 1 pause
