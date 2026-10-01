param(
    [string]$ApiBase = 'https://openapi.tripo3d.ai/v3',
    [string]$KeyFile = (Join-Path $PSScriptRoot '..\主角\tripo_api_key.txt'),
    [int]$PollSeconds = 8,
    [int]$TimeoutSeconds = 3600
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$model = 'v3.1-20260211'
$outputRoot = Join-Path $PSScriptRoot 'outputs'
$manifestPath = Join-Path $PSScriptRoot '房屋建筑部件生成清单.json'
$indexPath = Join-Path $PSScriptRoot '房屋建筑部件模型索引.md'
$assets = @(
    [ordered]@{ index = 1; name = '木地板模块'; slug = 'wood_floor_panel'; image = 'ChatGPT Image 2026年9月23日 12_30_10.png' },
    [ordered]@{ index = 2; name = '木楼梯模块'; slug = 'wood_stairs'; image = 'ChatGPT Image 2026年9月23日 12_30_16.png' },
    [ordered]@{ index = 3; name = '实心木墙模块'; slug = 'wood_wall_solid'; image = 'ChatGPT Image 2026年9月23日 12_30_22.png' },
    [ordered]@{ index = 4; name = '门洞木墙模块'; slug = 'wood_wall_doorway'; image = 'ChatGPT Image 2026年9月23日 12_30_27.png' },
    [ordered]@{ index = 5; name = '窗洞木墙模块'; slug = 'wood_wall_window'; image = 'ChatGPT Image 2026年9月23日 12_30_31.png' },
    [ordered]@{ index = 6; name = '木门'; slug = 'wood_door'; image = 'ChatGPT Image 2026年9月23日 12_30_36.png' },
    [ordered]@{ index = 7; name = '茅草坡屋顶模块'; slug = 'thatched_roof_slope'; image = 'ChatGPT Image 2026年9月23日 12_31_24.png' },
    [ordered]@{ index = 8; name = '茅草屋脊模块'; slug = 'thatched_roof_ridge'; image = 'ChatGPT Image 2026年9月23日 12_31_29.png' }
)

function Save-Manifest {
    $manifest | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $manifestPath -Encoding utf8
}

function Get-Data {
    param($Response, [string]$Operation)
    if ($null -eq $Response -or $Response.code -ne 0) {
        $detail = if ($null -eq $Response) { 'empty response' } else { $Response | ConvertTo-Json -Depth 5 -Compress }
        throw "$Operation failed: $detail"
    }
    return $Response.data
}

function Invoke-WithRetry {
    param([scriptblock]$Action, [string]$Operation, [int]$Attempts = 4)
    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        try { return & $Action } catch {
            if ($attempt -eq $Attempts) { throw }
            $delay = [Math]::Min(15, 2 * $attempt)
            Write-Warning ("{0}: attempt {1}/{2} failed; retrying in {3}s" -f $Operation, $attempt, $Attempts, $delay)
            Start-Sleep -Seconds $delay
        }
    }
}

function Test-Fbx {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    if ((Get-Item -LiteralPath $Path).Length -lt 1024) { return $false }
    $stream = [IO.File]::OpenRead($Path)
    try {
        $buffer = [byte[]]::new(20)
        [void]$stream.Read($buffer, 0, $buffer.Length)
        return [Text.Encoding]::ASCII.GetString($buffer) -eq 'Kaydara FBX Binary  '
    } finally { $stream.Dispose() }
}

function Get-Extension {
    param([string]$Key, [string]$Url)
    try { $ext = [IO.Path]::GetExtension(([Uri]$Url).AbsolutePath) } catch { $ext = '' }
    if ($ext) { return $ext.ToLowerInvariant() }
    if ($Key -match 'render|image|preview') { return '.webp' }
    return '.bin'
}

function Get-ImageExtension {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return '' }
    $stream = [IO.File]::OpenRead($Path)
    try {
        if ($stream.Length -lt 12) { return '' }
        $buffer = [byte[]]::new(12)
        [void]$stream.Read($buffer, 0, $buffer.Length)
        if ([BitConverter]::ToString($buffer, 0, 8) -eq '89-50-4E-47-0D-0A-1A-0A') { return '.png' }
        if ([Text.Encoding]::ASCII.GetString($buffer, 0, 4) -eq 'RIFF' -and
            [Text.Encoding]::ASCII.GetString($buffer, 8, 4) -eq 'WEBP') { return '.webp' }
        return ''
    } finally { $stream.Dispose() }
}

if (-not (Test-Path -LiteralPath $KeyFile -PathType Leaf)) { throw "API key file not found: $KeyFile" }
$apiKey = (Get-Content -LiteralPath $KeyFile -Raw).Trim()
if (-not $apiKey.StartsWith('tsk_')) { throw 'Tripo API key format is invalid.' }
$headers = @{ Authorization = "Bearer $apiKey" }
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null

if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json -AsHashtable
    if ($manifest.schema -ne 'tripo.building-parts.batch.v1') { throw 'Existing manifest has an unexpected schema.' }
    Write-Output 'Resuming existing batch.'
} else {
    $balanceResponse = Invoke-WithRetry -Operation 'Starting balance query' -Action {
        Invoke-RestMethod -Method Get -Uri "$ApiBase/account/balance" -Headers $headers -TimeoutSec 30
    }
    $balance = Get-Data -Response $balanceResponse -Operation 'Starting balance query'
    $manifest = [ordered]@{
        schema = 'tripo.building-parts.batch.v1'
        created_at = [DateTimeOffset]::UtcNow.ToString('o')
        api_base = $ApiBase
        settings = [ordered]@{
            model = $model; face_limit = 50000; texture = $true; pbr = $true
            texture_quality = 'detailed'; texture_alignment = 'original_image'
            orientation = 'align_image'; quad = $true
        }
        starting_balance = $balance
        assets = @()
    }
    Save-Manifest
    Write-Output ("Starting balance: {0}; frozen: {1}" -f $balance.balance, $balance.frozen)
}

foreach ($asset in $assets) {
    $imagePath = Join-Path $PSScriptRoot $asset.image
    if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf)) { throw "Input image not found: $imagePath" }
    $existing = @($manifest.assets | Where-Object { $_.index -eq $asset.index })
    if ($existing.Count -gt 1) { throw "Duplicate manifest entry for index $($asset.index)." }
    if ($existing.Count -eq 1) {
        if ($existing[0].image -ne $asset.image -or $existing[0].slug -ne $asset.slug) {
            throw "Manifest input mismatch for index $($asset.index)."
        }
        continue
    }

    Write-Output ("[{0}/8] Uploading {1}" -f $asset.index, $asset.name)
    $uploadResponse = Invoke-WithRetry -Operation "Upload $($asset.name)" -Action {
        Invoke-RestMethod -Method Post -Uri "$ApiBase/files" -Headers $headers -Form @{ file = Get-Item -LiteralPath $imagePath } -TimeoutSec 120
    }
    $upload = Get-Data -Response $uploadResponse -Operation "Upload $($asset.name)"
    $request = [ordered]@{
        input = $upload.file_token; model = $model; face_limit = 50000
        texture = $true; pbr = $true; texture_quality = 'detailed'
        texture_alignment = 'original_image'; orientation = 'align_image'; quad = $true
    }
    # Submitting can consume credits. Do not retry an ambiguous response automatically.
    $taskResponse = Invoke-RestMethod -Method Post -Uri "$ApiBase/generation/image-to-model" -Headers $headers -ContentType 'application/json' -Body ($request | ConvertTo-Json -Depth 10) -TimeoutSec 120
    $task = Get-Data -Response $taskResponse -Operation "Submit $($asset.name)"
    if (-not $task.task_id) { throw "Submission returned no task ID for $($asset.name)." }
    $manifest.assets += [ordered]@{
        index = $asset.index; name = $asset.name; slug = $asset.slug; image = $asset.image
        task_id = [string]$task.task_id; status = 'queued'; progress = 0
        credits_consumed = $null; downloaded_files = [ordered]@{}
    }
    Save-Manifest
    Write-Output ("[{0}/8] Submitted {1}: {2}" -f $asset.index, $asset.name, $task.task_id)
}

$deadline = [DateTimeOffset]::UtcNow.AddSeconds($TimeoutSeconds)
$lastStatus = @{}
while ($true) {
    $pending = @($manifest.assets | Where-Object {
        $taskDir = Join-Path $outputRoot $_.task_id
        $fbx = Join-Path $taskDir ($_.slug + '_model.fbx')
        $previewWebp = Join-Path $taskDir ($_.slug + '_rendered_image.webp')
        $previewPng = Join-Path $taskDir ($_.slug + '_rendered_image.png')
        $previewValid = (Get-ImageExtension -Path $previewWebp) -eq '.webp' -or
            (Get-ImageExtension -Path $previewPng) -eq '.png'
        $_.status -notin @('failed', 'cancelled') -and (
            -not (Test-Fbx -Path $fbx) -or
            -not $previewValid
        )
    })
    if ($pending.Count -eq 0) { break }
    if ([DateTimeOffset]::UtcNow -gt $deadline) {
        Save-Manifest
        throw "Timed out with $($pending.Count) unfinished model(s). Rerun the script to resume."
    }

    foreach ($record in $pending) {
        $taskResponse = Invoke-WithRetry -Operation "Query $($record.name)" -Action {
            Invoke-RestMethod -Method Get -Uri "$ApiBase/tasks/$($record.task_id)" -Headers $headers -TimeoutSec 60
        }
        $task = Get-Data -Response $taskResponse -Operation "Query $($record.name)"
        $record.status = [string]$task.status
        $record.progress = [int]$task.progress
        $state = "$($task.status):$($task.progress)"
        if ($lastStatus[$record.task_id] -ne $state) {
            Write-Output ("{0}: {1} ({2}%)" -f $record.name, $task.status, $task.progress)
            $lastStatus[$record.task_id] = $state
        }
        if ($task.status -in @('failed', 'cancelled')) {
            $record['error_code'] = $task.error_code
            $record['error_message'] = $task.error_message
            Save-Manifest
            continue
        }
        if ($task.status -ne 'success') { Save-Manifest; continue }

        $record.credits_consumed = $task.credits_consumed
        $taskDir = Join-Path $outputRoot $record.task_id
        New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
        foreach ($property in $task.output.PSObject.Properties) {
            if ($property.Name -notmatch '_url$' -or [string]::IsNullOrWhiteSpace([string]$property.Value)) { continue }
            $extension = Get-Extension -Key $property.Name -Url ([string]$property.Value)
            $fileName = '{0}_{1}{2}' -f $record.slug, ($property.Name -replace '_url$', ''), $extension
            $destination = Join-Path $taskDir $fileName
            $isValid = if ($property.Name -eq 'model_url') { Test-Fbx -Path $destination } else {
                (Get-ImageExtension -Path $destination) -ne ''
            }
            if (-not $isValid) {
                Write-Output ("Downloading {0}: {1}" -f $record.name, $fileName)
                Invoke-WithRetry -Operation "Download $($record.name) $($property.Name)" -Action {
                    Invoke-WebRequest -Uri ([string]$property.Value) -OutFile $destination -TimeoutSec 300
                } | Out-Null
            }
            if ($property.Name -ne 'model_url') {
                $actualExtension = Get-ImageExtension -Path $destination
                if (-not $actualExtension) { throw "Image validation failed for $($record.name): $destination" }
                if ($actualExtension -ne $extension) {
                    $fileName = '{0}_{1}{2}' -f $record.slug, ($property.Name -replace '_url$', ''), $actualExtension
                    $correctPath = Join-Path $taskDir $fileName
                    Move-Item -LiteralPath $destination -Destination $correctPath
                }
            }
            $record.downloaded_files[$property.Name] = "outputs/$($record.task_id)/$fileName"
        }
        $fbxPath = Join-Path $taskDir ($record.slug + '_model.fbx')
        if (-not (Test-Fbx -Path $fbxPath)) { throw "FBX validation failed for $($record.name): $fbxPath" }
        Save-Manifest
    }
    if (@($manifest.assets | Where-Object { $_.status -notin @('success', 'failed', 'cancelled') }).Count -gt 0) {
        Start-Sleep -Seconds $PollSeconds
    }
}

$balanceResponse = Invoke-WithRetry -Operation 'Ending balance query' -Action {
    Invoke-RestMethod -Method Get -Uri "$ApiBase/account/balance" -Headers $headers -TimeoutSec 30
}
$manifest['ending_balance'] = Get-Data -Response $balanceResponse -Operation 'Ending balance query'
$manifest['completed_at'] = [DateTimeOffset]::UtcNow.ToString('o')
Save-Manifest

$success = @($manifest.assets | Where-Object { $_.status -eq 'success' })
$credits = ($success | ForEach-Object { [decimal]$_.credits_consumed } | Measure-Object -Sum).Sum
$lines = @(
    '# 房屋建筑部件模型索引', ''
    "Tripo 模型：``$model``；50,000 面上限；详细 PBR 纹理；四边面拓扑；按原图视角对齐。"
    "完成：$($success.Count)/$($assets.Count)；消耗：$credits credits；余额：$($manifest.starting_balance.balance) → $($manifest.ending_balance.balance)。"
    ''
    '| # | 部件 | 原图 | FBX | 预览 | 任务 ID | 额度 |'
    '|---:|---|---|---|---|---|---:|'
)
foreach ($record in ($manifest.assets | Sort-Object index)) {
    $fbx = $record.downloaded_files.model_url
    $preview = $record.downloaded_files.rendered_image_url
    $fbxLink = if ($fbx) { "[FBX]($fbx)" } else { '未生成' }
    $previewLink = if ($preview) { "[预览]($preview)" } else { '无' }
    $lines += "| $($record.index) | $($record.name) | [$($record.image)](<$($record.image)>) | $fbxLink | $previewLink | ``$($record.task_id)`` | $($record.credits_consumed) |"
}
$lines += ''
$lines += '详细状态和本地文件路径见 [房屋建筑部件生成清单.json](房屋建筑部件生成清单.json)。这些是从单张视角图生成的静态候选模型，实际尺寸、背面、碰撞及模块拼接仍需在引擎中检验。'
$lines | Set-Content -LiteralPath $indexPath -Encoding utf8

if ($success.Count -ne $assets.Count) { throw "Only $($success.Count)/$($assets.Count) building parts succeeded. See $manifestPath" }
Write-Output ("All {0} building parts completed; credits consumed: {1}." -f $success.Count, $credits)
Write-Output "Index: $indexPath"
