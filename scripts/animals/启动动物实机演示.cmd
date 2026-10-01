@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0..\.." || exit /b 1
if defined HEARTHWARD_PYTHON (
  "%HEARTHWARD_PYTHON%" -X utf8 "%~dp0launch_demo.py"
) else (
  python -X utf8 "%~dp0launch_demo.py"
)
if errorlevel 1 (
  echo 动物演示启动失败，请查看 docs\qa\TASK-051 中的启动记录。
  pause
  exit /b 1
)
exit /b 0
