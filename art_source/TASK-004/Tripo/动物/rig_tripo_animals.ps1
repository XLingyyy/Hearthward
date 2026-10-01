param(
    [ValidateSet('Check', 'Rig')]
    [string]$Mode = 'Check',
    [string]$ApiBase = 'https://openapi.tripo3d.ai/v3',
    [string]$KeyFile = (Join-Path $PSScriptRoot '..\主角\tripo_api_key.txt'),
    [int]$PollSeconds = 5,
    [int]$TimeoutSeconds = 3600
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$generationManifestPath = Join-Path $PSScriptRoot '动物生成清单.json'
$rigManifestPath = Join-Path $PSScriptRoot '动物骨骼生成清单.json'
$outputRoot = Join-Path $PSScriptRoot 'rigged_outputs'
$rigModel = 'v2.5-20260210'
$terminalStates = @('success', 'failed', 'cancelled')

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

function Invoke-WithRetry {
    param(
        [Parameter(Mandatory)][scriptblock]$Action,
        [Parameter(Mandatory)][string]$Operation,
        [int]$Attempts = 5
    )
    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        try { return & $Action } catch {
            if ($attempt -eq $Attempts) { throw }
            $delay = [Math]::Min(20, 3 * $attempt)
            Write-Warning ("{0} attempt {1}/{2} failed; retrying in {3}s: {4}" -f $Operation, $attempt, $Attempts, $delay, $_.Exception.Message)
            Start-Sleep -Seconds $delay
        }
    }
}

function Get-Task {
    param([Parameter(Mandatory)][string]$TaskId, [Parameter(Mandatory)][string]$Label)
    $response = Invoke-WithRetry -Operation "Query $Label" -Action {
        Invoke-RestMethod -Method Get -Uri "$ApiBase/tasks/$TaskId" -Headers $script:headers -TimeoutSec 60
    }
    return Assert-TripoResponse -Response $response -Operation "Query $Label"
}

function Wait-TaskBatch {
    param(
        [Parameter(Mandatory)][object[]]$Records,
        [Parameter(Mandatory)][ValidateSet('Check', 'Rig')][string]$TaskKind
    )
    $deadline = [DateTimeOffset]::UtcNow.AddSeconds($TimeoutSeconds)
    $lastStatus = @{}
    while ($true) {
        $pending = if ($TaskKind -eq 'Check') {
            @($Records | Where-Object { $_.rig_check_status -notin $terminalStates })
        } else {
            @($Records | Where-Object { $_.rig_status -notin $terminalStates })
        }
        if ($pending.Count -eq 0) { break }
        if ([DateTimeOffset]::UtcNow -gt $deadline) {
            Save-Json -Value $script:manifest -Path $rigManifestPath
            throw "Timed out after $TimeoutSeconds seconds with $($pending.Count) unfinished $TaskKind task(s)."
        }

        foreach ($record in $pending) {
            $taskId = if ($TaskKind -eq 'Check') { $record.rig_check_task_id } else { $record.rig_task_id }
            $task = Get-Task -TaskId $taskId -Label "$TaskKind $($record.name)"
            $statusText = "$($task.status):$($task.progress)"
            if ($lastStatus[$taskId] -ne $statusText) {
                Write-Output ("{0} {1}: {2} ({3}%)" -f $TaskKind, $record.name, $task.status, $task.progress)
                $lastStatus[$taskId] = $statusText
            }
            if ($TaskKind -eq 'Check') {
                $record.rig_check_status = [string]$task.status
                if ($task.status -in $terminalStates) {
                    $record.rig_check_output = $task.output
                    $record.rig_check_credits = $task.credits_consumed
                    if ($task.output) {
                        $record.riggable = [bool]$task.output.riggable
                        $record.recommended_rig_type = [string]$task.output.rig_type
                    }
                }
            } else {
                $record.rig_status = [string]$task.status
                if ($task.status -in $terminalStates) {
                    $record.rig_output = $task.output
                    $record.rig_credits = $task.credits_consumed
                }
            }
        }
        Save-Json -Value $script:manifest -Path $rigManifestPath
        $remaining = if ($TaskKind -eq 'Check') {
            @($Records | Where-Object { $_.rig_check_status -notin $terminalStates }).Count
        } else {
            @($Records | Where-Object { $_.rig_status -notin $terminalStates }).Count
        }
        if ($remaining -gt 0) { Start-Sleep -Seconds $PollSeconds }
    }
}

if (-not (Test-Path -LiteralPath $KeyFile -PathType Leaf)) { throw "API key file not found: $KeyFile" }
$apiKey = (Get-Content -LiteralPath $KeyFile -Raw).Trim()
if (-not $apiKey.StartsWith('tsk_')) { throw 'Tripo API key format is invalid.' }
$script:headers = @{ Authorization = "Bearer $apiKey" }

if (-not (Test-Path -LiteralPath $generationManifestPath -PathType Leaf)) {
    throw "Generation manifest not found: $generationManifestPath"
}
$generationManifest = Get-Content -LiteralPath $generationManifestPath -Raw | ConvertFrom-Json
if (@($generationManifest.assets | Where-Object { $_.status -eq 'success' }).Count -ne 15) {
    throw 'Expected 15 successful source model tasks.'
}

if (Test-Path -LiteralPath $rigManifestPath -PathType Leaf) {
    $script:manifest = Get-Content -LiteralPath $rigManifestPath -Raw | ConvertFrom-Json
} else {
    $records = foreach ($asset in ($generationManifest.assets | Sort-Object index)) {
        [ordered]@{
            index = $asset.index
            name = $asset.name
            slug = $asset.slug
            source_task_id = $asset.task_id
            source_model = $asset.downloaded_files.model_url
            rig_check_task_id = $null
            rig_check_status = 'not_submitted'
            rig_check_output = $null
            rig_check_credits = $null
            riggable = $null
            recommended_rig_type = $null
            rig_task_id = $null
            rig_status = 'not_submitted'
            rig_request = $null
            rig_output = $null
            rig_credits = $null
            rigged_model = $null
        }
    }
    $script:manifest = [ordered]@{
        schema = 'tripo.animals.rig.batch.v3'
        created_at = [DateTimeOffset]::UtcNow.ToString('o')
        api_base = $ApiBase
        rig_model = $rigModel
        rig_spec = 'tripo'
        out_format = 'fbx'
        max_concurrency = 3
        starting_balance = $null
        ending_balance = $null
        assets = @($records)
    }
    Save-Json -Value $script:manifest -Path $rigManifestPath
}

if (@($script:manifest.assets).Count -ne 15) { throw 'Rig manifest does not contain exactly 15 assets.' }

if ($Mode -eq 'Check') {
    for ($offset = 0; $offset -lt 15; $offset += 3) {
        $batch = @($script:manifest.assets[$offset..([Math]::Min($offset + 2, 14))])
        foreach ($record in $batch) {
            if ($record.rig_check_status -eq 'success') { continue }
            if ([string]::IsNullOrWhiteSpace([string]$record.rig_check_task_id)) {
                $body = @{ input = $record.source_task_id } | ConvertTo-Json
                Write-Output ("[{0}/15] Submitting free rig check for {1}" -f $record.index, $record.name)
                $response = Invoke-WithRetry -Operation "Submit rig check $($record.name)" -Action {
                    Invoke-RestMethod -Method Post -Uri "$ApiBase/animations/rig-check" -Headers $script:headers -ContentType 'application/json' -Body $body -TimeoutSec 120
                }
                $task = Assert-TripoResponse -Response $response -Operation "Submit rig check $($record.name)"
                $record.rig_check_task_id = $task.task_id
                $record.rig_check_status = 'queued'
                Save-Json -Value $script:manifest -Path $rigManifestPath
            }
        }
        Wait-TaskBatch -Records $batch -TaskKind Check
    }

    $notRiggable = @($script:manifest.assets | Where-Object { $_.rig_check_status -ne 'success' -or -not $_.riggable })
    if ($notRiggable.Count -gt 0) {
        $notRiggable | ForEach-Object { Write-Error ("Rig check failed for {0}: status={1}, riggable={2}" -f $_.name, $_.rig_check_status, $_.riggable) }
        exit 1
    }
    $summary = $script:manifest.assets | Group-Object -Property { $_.recommended_rig_type } | Sort-Object Name | ForEach-Object { "$($_.Name)=$($_.Count)" }
    Write-Output ("All 15 rig checks passed: {0}" -f ($summary -join ', '))
    Write-Output "Manifest: $rigManifestPath"
    exit 0
}

$invalidChecks = @($script:manifest.assets | Where-Object { $_.rig_check_status -ne 'success' -or -not $_.riggable -or [string]::IsNullOrWhiteSpace([string]$_.recommended_rig_type) })
if ($invalidChecks.Count -gt 0) {
    throw 'All 15 successful rig checks are required before paid rigging. Run this script with -Mode Check first.'
}

New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$balanceResponse = Invoke-WithRetry -Operation 'Starting balance query' -Action {
    Invoke-RestMethod -Method Get -Uri "$ApiBase/account/balance" -Headers $script:headers -TimeoutSec 30
}
$script:manifest.starting_balance = Assert-TripoResponse -Response $balanceResponse -Operation 'Starting balance query'
Save-Json -Value $script:manifest -Path $rigManifestPath
Write-Output ("Starting balance: {0}" -f $script:manifest.starting_balance.balance)

for ($offset = 0; $offset -lt 15; $offset += 3) {
    $batch = @($script:manifest.assets[$offset..([Math]::Min($offset + 2, 14))])
    foreach ($record in $batch) {
        if ($record.rig_status -eq 'success') { continue }
        if ([string]::IsNullOrWhiteSpace([string]$record.rig_task_id)) {
            $request = [ordered]@{
                input = $record.source_task_id
                model = $rigModel
                rig_type = $record.recommended_rig_type
                spec = 'tripo'
                out_format = 'fbx'
            }
            $body = $request | ConvertTo-Json
            Write-Output ("[{0}/15] Submitting {1} rig for {2}" -f $record.index, $record.recommended_rig_type, $record.name)
            $response = Invoke-WithRetry -Operation "Submit rig $($record.name)" -Action {
                Invoke-RestMethod -Method Post -Uri "$ApiBase/animations/rig" -Headers $script:headers -ContentType 'application/json' -Body $body -TimeoutSec 120
            }
            $task = Assert-TripoResponse -Response $response -Operation "Submit rig $($record.name)"
            $record.rig_request = $request
            $record.rig_task_id = $task.task_id
            $record.rig_status = 'queued'
            Save-Json -Value $script:manifest -Path $rigManifestPath
        }
    }
    Wait-TaskBatch -Records $batch -TaskKind Rig
}

$failed = @($script:manifest.assets | Where-Object { $_.rig_status -ne 'success' })
if ($failed.Count -gt 0) {
    $failed | ForEach-Object { Write-Error ("Rig task failed for {0}: {1}" -f $_.name, $_.rig_status) }
    exit 1
}

foreach ($record in $script:manifest.assets) {
    if ([string]::IsNullOrWhiteSpace([string]$record.rig_output.model_url)) {
        throw "Rig task returned no model_url for $($record.name)."
    }
    $taskDir = Join-Path $outputRoot $record.rig_task_id
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $destination = Join-Path $taskDir ("{0}_rigged.fbx" -f $record.slug)
    if (-not (Test-Path -LiteralPath $destination -PathType Leaf)) {
        Write-Output ("Downloading rigged FBX: {0}" -f $record.name)
        Invoke-WithRetry -Operation "Download rigged $($record.name)" -Action {
            Invoke-WebRequest -Uri ([string]$record.rig_output.model_url) -OutFile $destination -TimeoutSec 300
        } | Out-Null
    }
    $record.rigged_model = $destination
    Save-Json -Value $record -Path (Join-Path $taskDir 'task.json')
    Save-Json -Value $script:manifest -Path $rigManifestPath
}

$endingBalanceResponse = Invoke-WithRetry -Operation 'Ending balance query' -Action {
    Invoke-RestMethod -Method Get -Uri "$ApiBase/account/balance" -Headers $script:headers -TimeoutSec 30
}
$script:manifest.ending_balance = Assert-TripoResponse -Response $endingBalanceResponse -Operation 'Ending balance query'
if ($null -eq $script:manifest.PSObject.Properties['completed_at']) {
    $script:manifest | Add-Member -NotePropertyName completed_at -NotePropertyValue $null
}
$script:manifest.completed_at = [DateTimeOffset]::UtcNow.ToString('o')
Save-Json -Value $script:manifest -Path $rigManifestPath

$credits = ($script:manifest.assets | ForEach-Object { [decimal]$_.rig_credits } | Measure-Object -Sum).Sum
Write-Output ("All 15 animal rigs completed; credits consumed: {0}." -f $credits)
Write-Output "Manifest: $rigManifestPath"
