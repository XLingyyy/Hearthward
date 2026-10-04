@echo off
cd /d "%~dp0"
python -X utf8 "%~dp0launch_pause_test.py"
if errorlevel 1 pause
