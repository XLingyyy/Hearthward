param(
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$projectDir = $PSScriptRoot
$python = Join-Path $projectDir '.venv\Scripts\python.exe'
$generator = Join-Path $projectDir 'tripo_generate.py'
$reference = Join-Path $projectDir 'reference\主角-中世纪哥特-正面.png'
$keyFile = Join-Path $projectDir '.tripo_api_key'
$keyTextFile = Join-Path $projectDir 'tripo_api_key.txt'

if (-not (Test-Path -LiteralPath $python -PathType Leaf)) {
    throw "未找到虚拟环境 Python：$python。请先按 README.md 安装依赖。"
}
if (-not (Test-Path -LiteralPath $reference -PathType Leaf)) {
    throw "未找到主角参考图：$reference"
}
if (-not $DryRun -and [string]::IsNullOrWhiteSpace($env:TRIPO_API_KEY)) {
    if (Test-Path -LiteralPath $keyFile -PathType Leaf) {
        $env:TRIPO_API_KEY = (Get-Content -LiteralPath $keyFile -Raw).Trim()
    } elseif (Test-Path -LiteralPath $keyTextFile -PathType Leaf) {
        $env:TRIPO_API_KEY = (Get-Content -LiteralPath $keyTextFile -Raw).Trim()
    }
}
if (-not $DryRun -and [string]::IsNullOrWhiteSpace($env:TRIPO_API_KEY)) {
    throw "未找到 API key。请设置 TRIPO_API_KEY，或把 key 单独写入：$keyFile"
}
if (-not $DryRun -and -not $env:TRIPO_API_KEY.StartsWith('tsk_')) {
    throw "Tripo API key 格式无效，应以 tsk_ 开头。"
}

$arguments = @($generator)
if ($DryRun) {
    $arguments += '--dry-run'
}
$arguments += @(
    'image',
    $reference,
    '--model', 'v3.1-20260211',
    '--face-limit', '80000',
    '--texture-quality', 'detailed',
    '--texture-alignment', 'original_image',
    '--orientation', 'align_image',
    '--quad'
)

& $python @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Tripo 主角生成失败，退出码：$LASTEXITCODE"
}
