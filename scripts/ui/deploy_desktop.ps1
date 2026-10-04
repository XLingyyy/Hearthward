param(
    [Parameter(Mandatory=$true)][string]$ArchiveRoot,
    [Parameter(Mandatory=$true)][string]$EvidenceRoot,
    [Parameter(Mandatory=$true)][ValidatePattern('^[a-z0-9_]+$')][string]$DeploymentId
)
$ErrorActionPreference = 'Stop'
$installRoot = 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2'
$archivePath = (Resolve-Path -LiteralPath $ArchiveRoot).Path
$evidencePath = (Resolve-Path -LiteralPath $EvidenceRoot).Path
$installPath = (Resolve-Path -LiteralPath $installRoot).Path
if ($installPath -ne $installRoot) { throw 'Unexpected desktop installation path.' }
$current = Join-Path $installPath 'Windows'
$candidate = Join-Path $installPath ('Windows-TASK053-' + $DeploymentId)
$backupRoot = Join-Path $installPath ('备份-TASK053-' + $DeploymentId)
$backup = Join-Path $backupRoot 'Windows'

function Assert-InstallPath([string]$Path) {
    $absolute = [IO.Path]::GetFullPath($Path)
    if (-not $absolute.StartsWith($installPath + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing file operation outside the approved installation: $absolute"
    }
    if ((Test-Path -LiteralPath $absolute) -and ((Get-Item -LiteralPath $absolute).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Refusing operation on a reparse point: $absolute"
    }
}
foreach ($path in @($current,$candidate,$backupRoot,$backup)) { Assert-InstallPath $path }
if (-not (Test-Path -LiteralPath (Join-Path $current 'Hearthward.exe'))) { throw 'Existing desktop game not found.' }
if ((Test-Path -LiteralPath $candidate) -or (Test-Path -LiteralPath $backupRoot)) { throw 'A candidate or program backup already exists; preserve it.' }
if (-not (Test-Path -LiteralPath (Join-Path $archivePath 'Hearthward.exe'))) { throw 'Shipping archive is incomplete.' }
$running = @(Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -and $_.ExecutablePath.StartsWith($current + '\',[StringComparison]::OrdinalIgnoreCase) })
if ($running.Count) { throw 'Close the existing desktop game before replacing its program directory.' }
$shortcutPath = Join-Path ([Environment]::GetFolderPath('Desktop')) '归火.lnk'
$shellLink = New-Object -ComObject WScript.Shell
$shortcut = $shellLink.CreateShortcut($shortcutPath)
$shortcutTarget = Join-Path $current 'Hearthward.exe'
$shortcutArguments = '-HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=16'
$reuseShortcut = Test-Path -LiteralPath $shortcutPath
if ($reuseShortcut -and ($shortcut.TargetPath -ne $shortcutTarget -or $shortcut.Arguments -ne $shortcutArguments -or $shortcut.WorkingDirectory -ne $current)) {
    throw 'Do not overwrite an existing unknown desktop shortcut.'
}
$playerRoot = Join-Path $env:LOCALAPPDATA 'Hearthward\Saved'
function Get-PlayerHashes {
    $hashes = @()
    if (Test-Path -LiteralPath $playerRoot) {
        foreach ($file in Get-ChildItem -LiteralPath $playerRoot -File -Recurse | Where-Object { $_.FullName -match '\\SaveGames\\|\\Config\\' } | Sort-Object FullName) {
            $relative = $file.FullName.Substring($playerRoot.Length + 1)
            $hashes += [pscustomobject]@{path=$relative;bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}
        }
    }
    return $hashes
}
$playerBefore = @(Get-PlayerHashes)
$sourceFiles = @(Get-ChildItem -LiteralPath $archivePath -File -Recurse)
if (@($sourceFiles | Where-Object { ($_.FullName.Substring($archivePath.Length + 1) -split '\\') -contains 'Saved' }).Count) { throw 'Archive contains Saved data; refuse to deploy developer profiles.' }
New-Item -ItemType Directory -Path $candidate | Out-Null
foreach ($item in Get-ChildItem -LiteralPath $archivePath) {
    Copy-Item -LiteralPath $item.FullName -Destination $candidate -Recurse
}
$verified = @()
foreach ($file in $sourceFiles) {
    $relative = $file.FullName.Substring($archivePath.Length + 1)
    $destination = Join-Path $candidate $relative
    $sourceHash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    if ($sourceHash -ne (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash) {
        throw "Copied file differs from the Shipping archive: $relative"
    }
    $verified += [pscustomobject]@{path=$relative;bytes=$file.Length;sha256=$sourceHash}
}
New-Item -ItemType Directory -Path $backupRoot | Out-Null
foreach ($file in $playerBefore) {
    $savedCopy = Join-Path (Join-Path $backupRoot 'PlayerSaved') $file.path
    New-Item -ItemType Directory -Path (Split-Path -Parent $savedCopy) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $playerRoot $file.path) -Destination $savedCopy
    if ((Get-FileHash -LiteralPath $savedCopy -Algorithm SHA256).Hash -ne $file.sha256) { throw 'Player backup differs from the current player data.' }
}

$previousInfo = Join-Path $installPath 'BUILD-INFO.json'
if (Test-Path -LiteralPath $previousInfo) {
    Copy-Item -LiteralPath $previousInfo -Destination (Join-Path $backupRoot 'BUILD-INFO.before.json')
}
# Both resolved move targets were checked above; no recursive deletion is used.
Move-Item -LiteralPath $current -Destination $backup
try {
    Move-Item -LiteralPath $candidate -Destination $current
} catch {
    if (-not (Test-Path -LiteralPath $current)) {
        Assert-InstallPath $backup
        Assert-InstallPath $current
        Move-Item -LiteralPath $backup -Destination $current
    }
    throw
}
Copy-Item -LiteralPath (Join-Path $current 'BUILD-INFO.json') -Destination $previousInfo
if (-not $reuseShortcut) {
    $shortcut.TargetPath = $shortcutTarget
    $shortcut.Arguments = $shortcutArguments
    $shortcut.WorkingDirectory = $current
    $shortcut.IconLocation = $shortcutTarget + ',0'
    $shortcut.Save()
}
$playerAfter = @(Get-PlayerHashes)
if (($playerBefore | ConvertTo-Json -Depth 4 -Compress) -ne ($playerAfter | ConvertTo-Json -Depth 4 -Compress)) {
    throw 'Player data changed during deployment; preserve the before/after evidence.'
}
$result = [pscustomobject]@{
    ok=$true
    installed=$current
    program_backup=$backup
    player_backup=(Join-Path $backupRoot 'PlayerSaved')
    shortcut=$shortcutPath
    verified_files=$verified
    developer_saved_data_deployed=$false
    player_before=$playerBefore
    player_after=$playerAfter
    reused_shortcut=$reuseShortcut
}
$json = $result | ConvertTo-Json -Depth 8
[IO.File]::WriteAllText((Join-Path $evidencePath 'deploy-result.json'), $json, (New-Object Text.UTF8Encoding($false)))
[pscustomobject]@{ok=$true;installed=$current;backup=$backup;shortcut=$shortcutPath;verified_files=$verified.Count} | ConvertTo-Json -Compress
