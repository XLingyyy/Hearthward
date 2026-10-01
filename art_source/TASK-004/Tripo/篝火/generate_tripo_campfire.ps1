param(
    [string]$ApiBase = 'https://openapi.tripo3d.ai/v3',
    [string]$KeyFile = (Join-Path $PSScriptRoot '..\主角\tripo_api_key.txt'),
    [int]$PollSeconds = 8,
    [int]$TimeoutSeconds = 3600
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$modelVersion = 'v3.1-20260211'
$imageName = 'ChatGPT Image 2026年9月23日 15_43_34.png'
$imagePath = Join-Path $PSScriptRoot $imageName
$outputDir = Join-Path $PSScriptRoot 'outputs'
$manifestPath = Join-Path $PSScriptRoot '篝火生成清单.json'
$indexPath = Join-Path $PSScriptRoot '篝火模型索引.md'
$fbxPath = Join-Path $outputDir 'campfire_model.fbx'

function Save-Manifest {
    $script:manifest | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $manifestPath -Encoding utf8
}

function Get-Data {
    param($Response, [string]$Operation)
    if ($null -eq $Response -or $Response.code -ne 0) {
        $code = if ($null -eq $Response) { 'empty response' } else { [string]$Response.code }
        throw "$Operation failed (Tripo code: $code)."
    }
    return $Response.data
}

function Invoke-TripoGet {
    param([string]$Uri, [string]$Operation)
    for ($attempt = 1; $attempt -le 4; $attempt++) {
        try {
            $response = Invoke-RestMethod -Method Get -Uri $Uri -Headers $script:headers -TimeoutSec 60
            return Get-Data -Response $response -Operation $Operation
        } catch {
            if ($attempt -eq 4) { throw }
            Start-Sleep -Seconds ([Math]::Min(15, 3 * $attempt))
        }
    }
}

function Test-Fbx {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    if ((Get-Item -LiteralPath $Path).Length -lt 1024) { return $false }
    $stream = [IO.File]::OpenRead($Path)
    try {
        $bytes = [byte[]]::new(20)
        [void]$stream.Read($bytes, 0, 20)
        return [Text.Encoding]::ASCII.GetString($bytes) -eq 'Kaydara FBX Binary  '
    } finally { $stream.Dispose() }
}

function Get-PreviewExtension {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return '' }
    $stream = [IO.File]::OpenRead($Path)
    try {
        if ($stream.Length -lt 12) { return '' }
        $bytes = [byte[]]::new(12)
        [void]$stream.Read($bytes, 0, 12)
        if ([BitConverter]::ToString($bytes, 0, 8) -eq '89-50-4E-47-0D-0A-1A-0A') { return '.png' }
        if ([Text.Encoding]::ASCII.GetString($bytes, 0, 4) -eq 'RIFF' -and [Text.Encoding]::ASCII.GetString($bytes, 8, 4) -eq 'WEBP') { return '.webp' }
        return ''
    } finally { $stream.Dispose() }
}

function Write-Index {
    $lines = @(
        '# 篝火模型索引', '',
        "Tripo 模型：``$modelVersion``；50,000 面上限；详细 PBR 纹理；四边面拓扑；按原图视角对齐。", '',
        "- 原图：[$imageName](<$imageName>)",
        "- 静态模型：[$($script:manifest.model_file)]($($script:manifest.model_file))",
        "- 预览：[$($script:manifest.preview_file)]($($script:manifest.preview_file))",
        "- Tripo 任务 ID：``$($script:manifest.task_id)``",
        "- 消耗：$($script:manifest.credits_consumed) credits；余额：$($script:manifest.starting_balance.balance) → $($script:manifest.ending_balance.balance)。", '',
        '[详细任务记录](篝火生成清单.json)。这是单图生成的静态候选模型；火焰动画、发光/透明材质、照明、碰撞、尺寸与背面仍需在 UE 中处理或检验。'
    )
    $lines | Set-Content -LiteralPath $indexPath -Encoding utf8
}

if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf)) { throw "Input image not found: $imagePath" }
if ((Get-Item -LiteralPath $imagePath).Length -gt 20MB) { throw 'Input image exceeds 20MB.' }
if ((Get-PreviewExtension -Path $imagePath) -ne '.png') { throw 'Input image is not a valid PNG.' }
if (-not (Test-Path -LiteralPath $KeyFile -PathType Leaf)) { throw "API key file not found: $KeyFile" }
$apiKey = (Get-Content -LiteralPath $KeyFile -Raw).Trim()
if (-not $apiKey.StartsWith('tsk_')) { throw 'Tripo API key format is invalid.' }
$script:headers = @{ Authorization = "Bearer $apiKey" }
$imageHash = (Get-FileHash -LiteralPath $imagePath -Algorithm SHA256).Hash
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
    $script:manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json -AsHashtable
    if ($script:manifest.schema -ne 'tripo.campfire.image-to-model.v1' -or $script:manifest.image_sha256 -ne $imageHash) {
        throw 'Existing manifest does not match the input image.'
    }
    Write-Output 'Resuming existing campfire task.'
} else {
    $balance = Invoke-TripoGet -Uri "$ApiBase/account/balance" -Operation 'Starting balance query'
    $script:manifest = [ordered]@{
        schema = 'tripo.campfire.image-to-model.v1'
        created_at = [DateTimeOffset]::UtcNow.ToString('o')
        api_base = $ApiBase
        model_version = $modelVersion
        image = $imageName
        image_sha256 = $imageHash
        settings = [ordered]@{ face_limit = 50000; texture = $true; pbr = $true; texture_quality = 'detailed'; texture_alignment = 'original_image'; orientation = 'align_image'; quad = $true }
        starting_balance = $balance
        submission_started_at = $null
        task_id = $null
        status = 'not_submitted'
        credits_consumed = $null
        model_file = $null
        preview_file = $null
        ending_balance = $null
    }
    Save-Manifest
    Write-Output ("Starting balance: {0}" -f $balance.balance)
}

if (-not $script:manifest.task_id) {
    if ($script:manifest.submission_started_at) { throw 'Prior submission outcome is uncertain; inspect Tripo tasks before retrying to avoid duplicate charges.' }
    $uploadResponse = Invoke-RestMethod -Method Post -Uri "$ApiBase/files" -Headers $script:headers -Form @{ file = Get-Item -LiteralPath $imagePath } -TimeoutSec 120
    $upload = Get-Data -Response $uploadResponse -Operation 'Upload campfire image'
    $request = [ordered]@{
        input = $upload.file_token; model = $modelVersion; face_limit = 50000
        texture = $true; pbr = $true; texture_quality = 'detailed'
        texture_alignment = 'original_image'; orientation = 'align_image'; quad = $true
    }
    $script:manifest.submission_started_at = [DateTimeOffset]::UtcNow.ToString('o')
    Save-Manifest
    # This POST can consume credits. Never retry an ambiguous submission automatically.
    $taskResponse = Invoke-RestMethod -Method Post -Uri "$ApiBase/generation/image-to-model" -Headers $script:headers -ContentType 'application/json' -Body ($request | ConvertTo-Json -Depth 10) -TimeoutSec 120
    $task = Get-Data -Response $taskResponse -Operation 'Submit campfire model'
    if (-not $task.task_id) { throw 'Submission returned no task ID.' }
    $script:manifest.task_id = [string]$task.task_id
    $script:manifest.status = 'queued'
    Save-Manifest
    Write-Output ("Submitted campfire task: {0}" -f $task.task_id)
}

$deadline = [DateTimeOffset]::UtcNow.AddSeconds($TimeoutSeconds)
$lastState = ''
while ($script:manifest.status -notin @('success', 'failed', 'cancelled', 'banned', 'expired')) {
    if ([DateTimeOffset]::UtcNow -gt $deadline) { throw 'Timed out; rerun script to resume.' }
    $task = Invoke-TripoGet -Uri "$ApiBase/tasks/$($script:manifest.task_id)" -Operation 'Query campfire task'
    $script:manifest.status = [string]$task.status
    $state = "$($task.status):$($task.progress)"
    if ($state -ne $lastState) { Write-Output ("Campfire: {0} ({1}%)" -f $task.status, $task.progress); $lastState = $state }
    if ($task.status -in @('success', 'failed', 'cancelled', 'banned', 'expired')) {
        $script:manifest.credits_consumed = $task.credits_consumed
        if ($task.status -ne 'success') { $script:manifest.error_code = $task.error_code; $script:manifest.error_message = $task.error_message }
    }
    Save-Manifest
    if ($task.status -notin @('success', 'failed', 'cancelled', 'banned', 'expired')) { Start-Sleep -Seconds $PollSeconds }
}
if ($script:manifest.status -ne 'success') { throw "Campfire generation did not succeed: $($script:manifest.status)." }

$task = Invoke-TripoGet -Uri "$ApiBase/tasks/$($script:manifest.task_id)" -Operation 'Get campfire output'
if (-not $task.output.model_url) { throw 'Completed task has no model URL.' }
if (-not (Test-Fbx -Path $fbxPath)) {
    $temporary = "$fbxPath.download"
    Invoke-WebRequest -Uri ([string]$task.output.model_url) -OutFile $temporary -TimeoutSec 300 | Out-Null
    if (-not (Test-Fbx -Path $temporary)) { throw 'Downloaded FBX is invalid.' }
    Move-Item -LiteralPath $temporary -Destination $fbxPath -Force
}
$script:manifest.model_file = 'outputs/campfire_model.fbx'
if ($task.output.rendered_image_url) {
    $png = Join-Path $outputDir 'campfire_preview.png'
    $webp = Join-Path $outputDir 'campfire_preview.webp'
    if ((Get-PreviewExtension -Path $png) -eq '.png') { $preview = $png }
    elseif ((Get-PreviewExtension -Path $webp) -eq '.webp') { $preview = $webp }
    else {
        $temporary = Join-Path $outputDir 'campfire_preview.download'
        Invoke-WebRequest -Uri ([string]$task.output.rendered_image_url) -OutFile $temporary -TimeoutSec 300 | Out-Null
        $extension = Get-PreviewExtension -Path $temporary
        if (-not $extension) { throw 'Downloaded preview is invalid.' }
        $preview = Join-Path $outputDir "campfire_preview$extension"
        Move-Item -LiteralPath $temporary -Destination $preview -Force
    }
    $script:manifest.preview_file = "outputs/$([IO.Path]::GetFileName($preview))"
}
$script:manifest.ending_balance = Invoke-TripoGet -Uri "$ApiBase/account/balance" -Operation 'Ending balance query'
$script:manifest.completed_at = [DateTimeOffset]::UtcNow.ToString('o')
Save-Manifest
Write-Index
Write-Output ("Campfire model complete; credits consumed: {0}; FBX: {1}" -f $script:manifest.credits_consumed, $fbxPath)
