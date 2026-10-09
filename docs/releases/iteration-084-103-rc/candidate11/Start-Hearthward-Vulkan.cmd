@echo off
setlocal
cd /d "%~dp0"
set "HEARTHWARD_CANDIDATE_PROFILE=%LOCALAPPDATA%\Hearthward\Candidates\iteration-084-103-20261009-11"
start "" "%~dp0Hearthward.exe" -HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=16 -UserDir="%HEARTHWARD_CANDIDATE_PROFILE%"
endlocal
