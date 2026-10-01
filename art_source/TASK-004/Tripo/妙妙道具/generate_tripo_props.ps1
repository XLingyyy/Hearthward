param(
    [string]$ApiBase = 'https://openapi.tripo3d.ai/v3',
    [string]$KeyFile = (Join-Path $PSScriptRoot '..\主角\tripo_api_key.txt'),
    [int]$PollSeconds = 5,
    [int]$TimeoutSeconds = 1800
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$model = 'v3.1-20260211'
$outputRoot = Join-Path $PSScriptRoot 'outputs'
$manifestPath = Join-Path $PSScriptRoot '妙妙道具生成清单.json'
$assets = @(
    [ordered]@{ index = 1; name = '石骨斧'; slug = 'stone_bone_axe'; image = 'ChatGPT Image 2026年9月22日 21_46_50 (1).png' },
    [ordered]@{ index = 2; name = '原始鱼竿'; slug = 'primitive_fishing_rod'; image = 'ChatGPT Image 2026年9月22日 21_46_50 (2).png' },
    [ordered]@{ index = 3; name = '骨肉袋'; slug = 'bone_meat_pouch'; image = 'ChatGPT Image 2026年9月22日 21_46_50 (3).png' },
    [ordered]@{ index = 4; name = '陶罐'; slug = 'clay_flask'; image = 'ChatGPT Image 2026年9月22日 21_46_50 (4).png' }
)

function Save-Json {
    param([Parameter(Mandatory)]$Value, [Parameter(Mandatory)][string]$Path)
    $Value | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $Path -Encoding utf8
}

function Assert-TripoResponse {
    param([Parameter(Mandatory)]$Response, [Parameter(Mandatory)][string]$Operation)
    if ($null -eq $Response -or $Response.code -ne 0) {
        $detail = if ($null -eq $Response) { 'empty response' } else { $Response | ConvertTo-Json -Depth 10 -Compress }
        throw "$Operation failed: $detail"
    }
    return $Response.data
}

function Get-DownloadExtension {
    param([string]$Key, [string]$Url)
    try {
        $extension = [IO.Path]::GetExtension(([Uri]$Url).AbsolutePath)
    } catch {
        $extension = ''
    }
    if ($extension) { return $extension.ToLowerInvariant() }
    if ($Key -match 'render|image|preview') { return '.webp' }
    return '.bin'
}

if (-not (Test-Path -LiteralPath $KeyFile -PathType Leaf)) {
    throw "API key file not found: $KeyFile"
}
$apiKey = (Get-Content -LiteralPath $KeyFile -Raw).Trim()
if (-not $apiKey.StartsWith('tsk_')) {
    throw 'Tripo API key format is invalid; expected a value beginning with tsk_.'
}
$headers = @{ Authorization = "Bearer $apiKey" }
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null

$balanceResponse = Invoke-RestMethod -Method Get -Uri "$ApiBase/account/balance" -Headers $headers -TimeoutSec 30
$balance = Assert-TripoResponse -Response $balanceResponse -Operation 'Balance query'
Write-Output ("Balance: {0}; frozen: {1}" -f $balance.balance, $balance.frozen)

$manifest = [ordered]@{
    schema = 'tripo.props.batch.v3'
    created_at = [DateTimeOffset]::UtcNow.ToString('o')
    api_base = $ApiBase
    settings = [ordered]@{
        model = $model
        face_limit = 50000
        texture = $true
        pbr = $true
        texture_quality = 'detailed'
        texture_alignment = 'original_image'
        orientation = 'align_image'
        quad = $true
    }
    starting_balance = $balance
    assets = @()
}

foreach ($asset in $assets) {
    $imagePath = Join-Path $PSScriptRoot $asset.image
    if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf)) {
        throw "Input image not found: $imagePath"
    }

    Write-Output ("[{0}/4] Uploading {1}: {2}" -f $asset.index, $asset.name, $asset.image)
    $uploadResponse = Invoke-RestMethod -Method Post -Uri "$ApiBase/files" -Headers $headers -Form @{ file = Get-Item -LiteralPath $imagePath } -TimeoutSec 120
    $upload = Assert-TripoResponse -Response $uploadResponse -Operation "Upload $($asset.name)"

    $request = [ordered]@{
        input = $upload.file_token
        model = $model
        face_limit = 50000
        texture = $true
        pbr = $true
        texture_quality = 'detailed'
        texture_alignment = 'original_image'
        orientation = 'align_image'
        quad = $true
    }
    $body = $request | ConvertTo-Json -Depth 10
    $taskResponse = Invoke-RestMethod -Method Post -Uri "$ApiBase/generation/image-to-model" -Headers $headers -ContentType 'application/json' -Body $body -TimeoutSec 120
    $task = Assert-TripoResponse -Response $taskResponse -Operation "Submit $($asset.name)"
    Write-Output ("[{0}/4] Submitted {1}: {2}" -f $asset.index, $asset.name, $task.task_id)

    $manifest.assets += [ordered]@{
        index = $asset.index
        name = $asset.name
        slug = $asset.slug
        image = $imagePath
        file_token = $upload.file_token
        task_id = $task.task_id
        request = $request
        status = 'queued'
        progress = 0
        output = $null
        credits_consumed = $null
        downloaded_files = [ordered]@{}
    }
    Save-Json -Value $manifest -Path $manifestPath
}

$deadline = [DateTimeOffset]::UtcNow.AddSeconds($TimeoutSeconds)
$lastStatus = @{}
while ($true) {
    $pending = @($manifest.assets | Where-Object { $_.status -notin @('success', 'failed', 'cancelled') })
    if ($pending.Count -eq 0) { break }
    if ([DateTimeOffset]::UtcNow -gt $deadline) {
        Save-Json -Value $manifest -Path $manifestPath
        throw "Timed out after $TimeoutSeconds seconds with $($pending.Count) unfinished task(s)."
    }

    foreach ($assetRecord in $pending) {
        $taskResponse = Invoke-RestMethod -Method Get -Uri "$ApiBase/tasks/$($assetRecord.task_id)" -Headers $headers -TimeoutSec 60
        $task = Assert-TripoResponse -Response $taskResponse -Operation "Query $($assetRecord.name)"
        $assetRecord.status = [string]$task.status
        $assetRecord.progress = [int]$task.progress
        $statusText = "$($task.status):$($task.progress)"
        if ($lastStatus[$assetRecord.task_id] -ne $statusText) {
            Write-Output ("{0}: {1} ({2}%)" -f $assetRecord.name, $task.status, $task.progress)
            $lastStatus[$assetRecord.task_id] = $statusText
        }
        if ($task.status -in @('success', 'failed', 'cancelled')) {
            $assetRecord.output = $task.output
            $assetRecord.credits_consumed = $task.credits_consumed
            if ($task.error_code) { $assetRecord.error_code = $task.error_code }
            if ($task.error_message) { $assetRecord.error_message = $task.error_message }
        }
    }
    Save-Json -Value $manifest -Path $manifestPath
    if (@($manifest.assets | Where-Object { $_.status -notin @('success', 'failed', 'cancelled') }).Count -gt 0) {
        Start-Sleep -Seconds $PollSeconds
    }
}

foreach ($assetRecord in $manifest.assets) {
    if ($assetRecord.status -ne 'success') { continue }
    $taskDir = Join-Path $outputRoot $assetRecord.task_id
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    foreach ($property in $assetRecord.output.PSObject.Properties) {
        if ($property.Name -notmatch '_url$' -or [string]::IsNullOrWhiteSpace([string]$property.Value)) { continue }
        $extension = Get-DownloadExtension -Key $property.Name -Url ([string]$property.Value)
        $fileName = '{0}_{1}{2}' -f $assetRecord.slug, ($property.Name -replace '_url$', ''), $extension
        $destination = Join-Path $taskDir $fileName
        Write-Output ("Downloading {0} -> {1}" -f $property.Name, $destination)
        Invoke-WebRequest -Uri ([string]$property.Value) -OutFile $destination -TimeoutSec 300
        $assetRecord.downloaded_files[$property.Name] = $destination
    }
    Save-Json -Value $assetRecord -Path (Join-Path $taskDir 'task.json')
}

$endingBalanceResponse = Invoke-RestMethod -Method Get -Uri "$ApiBase/account/balance" -Headers $headers -TimeoutSec 30
$manifest.ending_balance = Assert-TripoResponse -Response $endingBalanceResponse -Operation 'Ending balance query'
$manifest.completed_at = [DateTimeOffset]::UtcNow.ToString('o')
Save-Json -Value $manifest -Path $manifestPath

$failed = @($manifest.assets | Where-Object { $_.status -ne 'success' })
if ($failed.Count -gt 0) {
    Write-Error "$($failed.Count) task(s) did not succeed. See $manifestPath"
    exit 1
}

Write-Output "All four props completed successfully."
Write-Output "Manifest: $manifestPath"
