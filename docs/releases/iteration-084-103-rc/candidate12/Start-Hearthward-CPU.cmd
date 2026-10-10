@echo off
setlocal
cd /d "%~dp0"
set "HEARTHWARD_CANDIDATE_PROFILE=%LOCALAPPDATA%\Hearthward\Candidates\iteration-084-103-20261009-12"
start "" "%~dp0Hearthward.exe" -HearthwardAIBackend=cpu -HearthwardAIGpuLayers=16 -UserDir="%HEARTHWARD_CANDIDATE_PROFILE%"
endlocal
