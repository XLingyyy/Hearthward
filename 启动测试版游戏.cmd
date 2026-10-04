@echo off
chcp 65001 >nul
call "%~dp0TestClient\启动测试端.cmd" %*
