$ErrorActionPreference = 'Stop'
$projectDir = $PSScriptRoot
$keyFile = Join-Path $projectDir '.tripo_api_key'
$keyTextFile = Join-Path $projectDir 'tripo_api_key.txt'
$python = Join-Path $projectDir '.venv\Scripts\python.exe'
$script = Join-Path $projectDir 'tripo_rig_character.py'
$sourceTaskId = '18e5e1ad-4309-4b66-b9a6-8b499cbaa7f4'

if ([string]::IsNullOrWhiteSpace($env:TRIPO_API_KEY)) {
    if (Test-Path -LiteralPath $keyFile -PathType Leaf) {
        $env:TRIPO_API_KEY = (Get-Content -LiteralPath $keyFile -Raw).Trim()
    } elseif (Test-Path -LiteralPath $keyTextFile -PathType Leaf) {
        $env:TRIPO_API_KEY = (Get-Content -LiteralPath $keyTextFile -Raw).Trim()
    }
}
if ([string]::IsNullOrWhiteSpace($env:TRIPO_API_KEY)) {
    throw '未找到 Tripo API key。'
}
if (-not $env:TRIPO_API_KEY.StartsWith('tsk_')) {
    throw 'Tripo API key 格式无效。'
}

& $python $script $sourceTaskId
if ($LASTEXITCODE -ne 0) {
    throw "Tripo 骨骼绑定失败，退出码：$LASTEXITCODE"
}
