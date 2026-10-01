param(
    [ValidateSet('All', 'Generate', 'Check', 'Rig', 'Verify')]
    [string]$Stage = 'All',
    [string]$ApiBase = 'https://openapi.tripo3d.ai/v3',
    [string]$KeyFile = (Join-Path $PSScriptRoot '..\主角\tripo_api_key.txt'),
    [int]$PollSeconds = 8,
    [int]$TimeoutSeconds = 3600
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$manifestPath = Join-Path $PSScriptRoot '敌人生成与骨骼清单.json'
$indexPath = Join-Path $PSScriptRoot '敌人模型与骨骼索引.md'
$outputRoot = Join-Path $PSScriptRoot 'outputs'
$modelVersion = 'v3.1-20260211'
$rigVersion = 'v2.5-20260210'
$terminalStatuses = @('success', 'failed', 'cancelled', 'banned', 'expired')
$sourceAssets = @(
    [ordered]@{ index = 1; name = '短刀兵'; slug = 'short_blade_soldier'; image = '短刀兵/ChatGPT Image 2026年9月23日 15_43_01.png' },
    [ordered]@{ index = 2; name = '重甲兵'; slug = 'heavy_armored_soldier'; image = '重甲兵/ChatGPT Image 2026年9月23日 15_43_07.png' }
)

function Save-Manifest {
    $script:manifest | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $manifestPath -Encoding utf8
}

function Get-TripoData {
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
            return Get-TripoData -Response $response -Operation $Operation
        } catch {
            if ($attempt -eq 4) { throw }
            Start-Sleep -Seconds ([Math]::Min(15, 3 * $attempt))
        }
    }
}

function Get-Balance {
    Invoke-TripoGet -Uri "$ApiBase/account/balance" -Operation 'Balance query'
}

function Submit-Once {
    param([System.Collections.IDictionary]$Record, [string]$Prefix, [string]$Uri, $Body)
    $taskIdKey = "${Prefix}_task_id"
    $startedKey = "${Prefix}_submission_started_at"
    if ($Record[$taskIdKey]) { return }
    if ($Record[$startedKey]) {
        throw "Submission for $($Record.name) ($Prefix) is uncertain. Inspect Tripo tasks before retrying to avoid duplicate charges."
    }
    $Record[$startedKey] = [DateTimeOffset]::UtcNow.ToString('o')
    Save-Manifest
    # A lost POST response could conceal a charged task. Do not retry it automatically.
    $response = Invoke-RestMethod -Method Post -Uri $Uri -Headers $script:headers -ContentType 'application/json' -Body ($Body | ConvertTo-Json -Depth 10) -TimeoutSec 120
    $task = Get-TripoData -Response $response -Operation "Submit $Prefix $($Record.name)"
    if ([string]::IsNullOrWhiteSpace([string]$task.task_id)) { throw "No task ID returned for $Prefix $($Record.name)." }
    $Record[$taskIdKey] = [string]$task.task_id
    $Record["${Prefix}_status"] = 'queued'
    Save-Manifest
    Write-Output ("Submitted {0} for {1}: {2}" -f $Prefix, $Record.name, $task.task_id)
}

function Wait-Tasks {
    param([string]$Prefix)
    $deadline = [DateTimeOffset]::UtcNow.AddSeconds($TimeoutSeconds)
    $lastState = @{}
    while ($true) {
        $pending = @($script:manifest.assets | Where-Object { $_["${Prefix}_task_id"] -and $_["${Prefix}_status"] -notin $terminalStatuses })
        if ($pending.Count -eq 0) { break }
        if ([DateTimeOffset]::UtcNow -gt $deadline) { throw "Timed out waiting for $Prefix tasks; rerun to resume." }
        foreach ($record in $pending) {
            $task = Invoke-TripoGet -Uri "$ApiBase/tasks/$($record["${Prefix}_task_id"])" -Operation "Query $Prefix $($record.name)"
            $record["${Prefix}_status"] = [string]$task.status
            $state = "$($task.status):$($task.progress)"
            if ($lastState[$record.slug] -ne $state) {
                Write-Output ("{0} {1}: {2} ({3}%)" -f $Prefix, $record.name, $task.status, $task.progress)
                $lastState[$record.slug] = $state
            }
            if ($task.status -in $terminalStatuses) {
                $record["${Prefix}_credits"] = $task.credits_consumed
                if ($Prefix -eq 'check') {
                    $record.riggable = [bool]$task.output.riggable
                    $record.rig_type = [string]$task.output.rig_type
                }
                if ($task.status -ne 'success') {
                    $record["${Prefix}_error_code"] = $task.error_code
                    $record["${Prefix}_error_message"] = $task.error_message
                }
            }
            Save-Manifest
        }
        if (@($script:manifest.assets | Where-Object { $_["${Prefix}_task_id"] -and $_["${Prefix}_status"] -notin $terminalStatuses }).Count -gt 0) {
            Start-Sleep -Seconds $PollSeconds
        }
    }
    $failed = @($script:manifest.assets | Where-Object { $_["${Prefix}_status"] -ne 'success' })
    if ($failed.Count -gt 0) { throw "$Prefix failed or was not submitted for: $(($failed.name) -join ', ')." }
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

function Get-ImageExtension {
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

function Download-Model {
    param([System.Collections.IDictionary]$Record, [ValidateSet('generation', 'rig')][string]$Kind)
    $taskId = $Record["${Kind}_task_id"]
    $task = Invoke-TripoGet -Uri "$ApiBase/tasks/$taskId" -Operation "Get $Kind output for $($Record.name)"
    if ($task.status -ne 'success' -or [string]::IsNullOrWhiteSpace([string]$task.output.model_url)) {
        throw "Successful $Kind task has no model_url for $($Record.name)."
    }
    $assetDir = Join-Path $outputRoot $Record.slug
    New-Item -ItemType Directory -Path $assetDir -Force | Out-Null
    $fileName = if ($Kind -eq 'rig') { "$($Record.slug)_rigged.fbx" } else { "$($Record.slug)_model.fbx" }
    $destination = Join-Path $assetDir $fileName
    if (-not (Test-Fbx -Path $destination)) {
        $temporary = "$destination.download"
        Invoke-WebRequest -Uri ([string]$task.output.model_url) -OutFile $temporary -TimeoutSec 300 | Out-Null
        if (-not (Test-Fbx -Path $temporary)) { throw "Downloaded $Kind FBX is invalid for $($Record.name)." }
        Move-Item -LiteralPath $temporary -Destination $destination -Force
    }
    $Record["${Kind}_model"] = "outputs/$($Record.slug)/$fileName"
    if ($Kind -eq 'generation' -and $task.output.rendered_image_url) {
        $png = Join-Path $assetDir "$($Record.slug)_preview.png"
        $webp = Join-Path $assetDir "$($Record.slug)_preview.webp"
        if ((Get-ImageExtension -Path $png) -eq '.png') { $preview = $png }
        elseif ((Get-ImageExtension -Path $webp) -eq '.webp') { $preview = $webp }
        else {
            $temporary = Join-Path $assetDir "$($Record.slug)_preview.download"
            Invoke-WebRequest -Uri ([string]$task.output.rendered_image_url) -OutFile $temporary -TimeoutSec 300 | Out-Null
            $extension = Get-ImageExtension -Path $temporary
            if (-not $extension) { throw "Downloaded preview is invalid for $($Record.name)." }
            $preview = Join-Path $assetDir "$($Record.slug)_preview$extension"
            Move-Item -LiteralPath $temporary -Destination $preview -Force
        }
        $Record.preview = "outputs/$($Record.slug)/$([IO.Path]::GetFileName($preview))"
    }
    Save-Manifest
}

function Write-Index {
    $lines = @(
        '# 敌人模型与骨骼索引', '',
        "建模版本：``$modelVersion``；50,000 面上限；详细 PBR 纹理；四边面拓扑。绑骨版本：``$rigVersion``；Tripo 骨骼；FBX 输出。", '',
        '| 人物 | 原图 | 静态 FBX | 预览 | 带骨骼 FBX | Rig Check |',
        '|---|---|---|---|---|---|'
    )
    foreach ($record in $script:manifest.assets) {
        $image = "[$($record.name) 原图](<$($record.image)>)"
        $static = if ($record.generation_model) { "[FBX]($($record.generation_model))" } else { '未完成' }
        $preview = if ($record.preview) { "[预览]($($record.preview))" } else { '未完成' }
        $rigged = if ($record.rig_model) { "[FBX]($($record.rig_model))" } else { '未完成' }
        $check = if ($record.check_status -eq 'success') { "$($record.riggable) / $($record.rig_type)" } else { $record.check_status }
        $lines += "| $($record.name) | $image | $static | $preview | $rigged | $check |"
    }
    $lines += ''
    $lines += '[任务、状态与额度记录](敌人生成与骨骼清单.json)。单张图像重建的背面、护甲穿插、刀/锤与盾牌的绑定权重及游戏内动画仍须在 DCC/UE 中检查；带骨骼 FBX 不等于已完成游戏动画。'
    $lines | Set-Content -LiteralPath $indexPath -Encoding utf8
}

foreach ($asset in $sourceAssets) {
    $imagePath = Join-Path $PSScriptRoot $asset.image
    if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf)) { throw "Input image not found: $imagePath" }
    if ((Get-Item -LiteralPath $imagePath).Length -gt 20MB) { throw "Input image exceeds 20MB: $imagePath" }
    if ((Get-ImageExtension -Path $imagePath) -ne '.png') { throw "Input is not PNG: $imagePath" }
}
if ($Stage -ne 'Verify') {
    if (-not (Test-Path -LiteralPath $KeyFile -PathType Leaf)) { throw "API key file not found: $KeyFile" }
    $apiKey = (Get-Content -LiteralPath $KeyFile -Raw).Trim()
    if (-not $apiKey.StartsWith('tsk_')) { throw 'Tripo API key format is invalid.' }
    $script:headers = @{ Authorization = "Bearer $apiKey" }
}

if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
    $script:manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json -AsHashtable
    if ($script:manifest.schema -ne 'tripo.enemies.generate-and-rig.v1' -or @($script:manifest.assets).Count -ne 2) { throw 'Manifest schema or asset count mismatch.' }
    foreach ($asset in $sourceAssets) {
        $record = @($script:manifest.assets | Where-Object { $_.index -eq $asset.index })
        if ($record.Count -ne 1 -or $record[0].image -ne $asset.image -or $record[0].slug -ne $asset.slug) { throw "Manifest input mismatch: $($asset.name)" }
        if ($record[0].image_sha256 -ne (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $asset.image) -Algorithm SHA256).Hash) { throw "Input image changed: $($asset.name)" }
    }
} else {
    if ($Stage -in @('Check', 'Rig', 'Verify')) { throw 'Generation manifest not found; run Generate first.' }
    $records = foreach ($asset in $sourceAssets) {
        [ordered]@{
            index = $asset.index; name = $asset.name; slug = $asset.slug; image = $asset.image
            image_sha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $asset.image) -Algorithm SHA256).Hash
            generation_submission_started_at = $null; generation_task_id = $null; generation_status = 'not_submitted'; generation_credits = $null
            generation_model = $null; preview = $null
            check_submission_started_at = $null; check_task_id = $null; check_status = 'not_submitted'; check_credits = $null
            riggable = $null; rig_type = $null
            rig_submission_started_at = $null; rig_task_id = $null; rig_status = 'not_submitted'; rig_credits = $null; rig_model = $null
        }
    }
    $script:manifest = [ordered]@{
        schema = 'tripo.enemies.generate-and-rig.v1'; created_at = [DateTimeOffset]::UtcNow.ToString('o')
        api_base = $ApiBase; generation_model = $modelVersion; rig_version = $rigVersion
        starting_balance = Get-Balance; ending_balance = $null; assets = @($records)
    }
    Save-Manifest
    Write-Output ("Starting balance: {0}" -f $script:manifest.starting_balance.balance)
}

if ($Stage -in @('All', 'Generate')) {
    foreach ($record in $script:manifest.assets) {
        if ($record.generation_status -eq 'success') { continue }
        if (-not $record.generation_task_id) {
            $imagePath = Join-Path $PSScriptRoot $record.image
            $uploadResponse = Invoke-RestMethod -Method Post -Uri "$ApiBase/files" -Headers $script:headers -Form @{ file = Get-Item -LiteralPath $imagePath } -TimeoutSec 120
            $upload = Get-TripoData -Response $uploadResponse -Operation "Upload $($record.name)"
            $request = [ordered]@{
                input = $upload.file_token; model = $modelVersion; face_limit = 50000
                texture = $true; pbr = $true; texture_quality = 'detailed'
                texture_alignment = 'original_image'; orientation = 'align_image'; quad = $true
            }
            Submit-Once -Record $record -Prefix generation -Uri "$ApiBase/generation/image-to-model" -Body $request
        }
    }
    Wait-Tasks -Prefix generation
    foreach ($record in $script:manifest.assets) { Download-Model -Record $record -Kind generation }
    Write-Index
    Write-Output 'Both static enemy models are complete.'
}

if ($Stage -in @('All', 'Check')) {
    if (@($script:manifest.assets | Where-Object { $_.generation_status -ne 'success' }).Count -gt 0) { throw 'Both source models must succeed before rig checks.' }
    foreach ($record in $script:manifest.assets) {
        if ($record.check_status -eq 'success') { continue }
        Submit-Once -Record $record -Prefix check -Uri "$ApiBase/animations/rig-check" -Body @{ input = $record.generation_task_id }
    }
    Wait-Tasks -Prefix check
    foreach ($record in $script:manifest.assets) {
        if (-not $record.riggable -or $record.rig_type -ne 'biped') { throw "Rig check rejected biped skeleton for $($record.name): riggable=$($record.riggable), type=$($record.rig_type)." }
    }
    Write-Index
    Write-Output 'Both biped rig checks passed.'
}

if ($Stage -in @('All', 'Rig')) {
    foreach ($record in $script:manifest.assets) {
        if ($record.check_status -ne 'success' -or -not $record.riggable -or $record.rig_type -ne 'biped') { throw "Biped rig check is required for $($record.name)." }
        if ($record.rig_status -eq 'success') { continue }
        $request = [ordered]@{ input = $record.generation_task_id; model = $rigVersion; rig_type = 'biped'; spec = 'tripo'; out_format = 'fbx' }
        Submit-Once -Record $record -Prefix rig -Uri "$ApiBase/animations/rig" -Body $request
    }
    Wait-Tasks -Prefix rig
    foreach ($record in $script:manifest.assets) { Download-Model -Record $record -Kind rig }
    $script:manifest.ending_balance = Get-Balance
    $script:manifest.completed_at = [DateTimeOffset]::UtcNow.ToString('o')
    Save-Manifest
    Write-Index
    Write-Output 'Both rigged enemy models are complete.'
}

if ($Stage -eq 'Verify') {
    foreach ($record in $script:manifest.assets) {
        $static = Join-Path $PSScriptRoot $record.generation_model
        $rigged = Join-Path $PSScriptRoot $record.rig_model
        $preview = Join-Path $PSScriptRoot $record.preview
        if (-not (Test-Fbx -Path $static) -or -not (Test-Fbx -Path $rigged) -or -not (Get-ImageExtension -Path $preview)) { throw "Invalid or missing outputs for $($record.name)." }
        Write-Output ("Verified files: {0}; static={1}; rigged={2}; preview={3}" -f $record.name, (Get-Item -LiteralPath $static).Length, (Get-Item -LiteralPath $rigged).Length, (Get-Item -LiteralPath $preview).Length)
    }
    Write-Index
}
