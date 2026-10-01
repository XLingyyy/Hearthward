$ErrorActionPreference = 'Stop'
$projectDir = $PSScriptRoot
$keyFile = Join-Path $projectDir '.tripo_api_key'
$keyTextFile = Join-Path $projectDir 'tripo_api_key.txt'
$python = Join-Path $projectDir '.venv\Scripts\python.exe'
$script = Join-Path $projectDir 'tripo_retarget_character.py'
$rigTaskId = '2a9cf835-4661-4ac7-962e-c40ed8608953'

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

& $python $script $rigTaskId
if ($LASTEXITCODE -ne 0) {
    throw "Tripo 动画生成失败，退出码：$LASTEXITCODE"
}
