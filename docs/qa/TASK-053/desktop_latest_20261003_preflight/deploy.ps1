$ErrorActionPreference = 'Stop'
$gameRoot = 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2'
$install = Join-Path $gameRoot 'Windows'
$backup = Join-Path $gameRoot '备份-TASK053-20261003_latest_ui'
$oldProgram = Join-Path $backup 'Windows'
$candidate = Join-Path $gameRoot 'Windows-TASK053-latest-ui-ready'
$failed = Join-Path $gameRoot 'Windows-TASK053-latest-ui-failed'
$playerRoot = 'C:\Users\22543\AppData\Local\Hearthward\Saved'
$workspace = 'E:\AiAgent\XLingGame\Hearthward'
$evidence = Join-Path $workspace 'docs\qa\TASK-053\desktop_20261003_latest_ui'
$beforePath = Join-Path $workspace 'docs\qa\TASK-053\desktop_latest_20261003_preflight\before.json'
$archive = Join-Path $workspace 'Saved\UIShipping\TASK-053\desktop_20261003_latest_ui\Archive\Windows'
function Assert-Path([string]$path,[string]$exact) {
    if ([IO.Path]::GetFullPath($path) -ne [IO.Path]::GetFullPath($exact)) { throw 'Resolved path differs from explicit target' }
    if (-not [IO.Path]::GetFullPath($path).StartsWith($gameRoot+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Target escapes explicitly named installation directory' }
    if ((Test-Path -LiteralPath $path) -and ((Get-Item -LiteralPath $path).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Target is a reparse point' }
}
function Get-Manifest([string]$root) {
    $absolute=[IO.Path]::GetFullPath($root).TrimEnd('\')
    if (-not (Test-Path -LiteralPath $absolute)) { return @() }
    return @(Get-ChildItem -LiteralPath $absolute -Recurse -File | Sort-Object FullName | ForEach-Object {
        [pscustomobject]@{path=$_.FullName.Substring($absolute.Length+1).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
    })
}
function Get-PlayerManifest([string]$root) {
    return @(foreach ($name in @('SaveGames','Config')) {
        foreach ($entry in @(Get-Manifest (Join-Path $root $name))) {
            [pscustomobject]@{path=$name+'/'+$entry.path;bytes=$entry.bytes;sha256=$entry.sha256}
        }
    })
}
function Equal-Manifest($a,$b) { return (($a|ConvertTo-Json -Depth 8 -Compress) -eq ($b|ConvertTo-Json -Depth 8 -Compress)) }
function Write-Json([string]$path,$data) { [IO.File]::WriteAllText($path,($data|ConvertTo-Json -Depth 15),[Text.UTF8Encoding]::new($false)) }

Assert-Path $install 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\Windows'
Assert-Path $candidate 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\Windows-TASK053-latest-ui-ready'
Assert-Path $oldProgram 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\备份-TASK053-20261003_latest_ui\Windows'
Assert-Path $failed 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\Windows-TASK053-latest-ui-failed'
if (-not [IO.Path]::GetFullPath($archive).StartsWith($workspace+'\Saved\UIShipping\TASK-053\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected archive path' }
if ((Get-Item -LiteralPath $gameRoot).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Installation root is a reparse point' }
if ((Get-Item -LiteralPath $backup).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Backup root is a reparse point' }
if ((Test-Path -LiteralPath $candidate) -or (Test-Path -LiteralPath $oldProgram) -or (Test-Path -LiteralPath $failed)) { throw 'Preserve prior candidate/backup files' }
$active=@(Get-CimInstance Win32_Process|Where-Object { $_.Name -match 'Hearthward|UnrealEditor' })
if ($active.Count) { throw 'Game/editor became active; preserve the running session' }
$before=Get-Content -LiteralPath $beforePath -Raw -Encoding UTF8|ConvertFrom-Json
$validated=Get-Content -LiteralPath (Join-Path $evidence 'archive-validation.json') -Raw -Encoding UTF8|ConvertFrom-Json
if (-not $validated.passed) { throw 'Archive has not passed validation' }
if (-not (Equal-Manifest @(Get-Manifest $install) @($before.program))) { throw 'Installed game changed after preflight' }
if (-not (Equal-Manifest @(Get-PlayerManifest $playerRoot) @($before.player))) { throw 'Player data changed after backup' }
if (-not (Equal-Manifest @(Get-PlayerManifest (Join-Path $backup 'PlayerSaved')) @($before.player))) { throw 'Player backup differs from preflight' }

$source=@(Get-Manifest $archive)
$requiredBytes=($source|Measure-Object -Property bytes -Sum).Sum+1GB
if ([IO.DriveInfo]::new('C:\').AvailableFreeSpace -lt $requiredBytes) { throw 'Not enough free space for a complete verified candidate' }
Copy-Item -LiteralPath $archive -Destination $candidate -Recurse
if (-not (Equal-Manifest $source @(Get-Manifest $candidate))) { throw 'Candidate copy does not match package' }
# Check all resolved absolute paths immediately before each recursive directory move.
Assert-Path $install 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\Windows'
Assert-Path $oldProgram 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\备份-TASK053-20261003_latest_ui\Windows'
Move-Item -LiteralPath $install -Destination $oldProgram
try {
    if (-not (Equal-Manifest @(Get-Manifest $oldProgram) @($before.program))) { throw 'Old program backup differs' }
    Assert-Path $candidate 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\Windows-TASK053-latest-ui-ready'
    Assert-Path $install 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\Windows'
    Move-Item -LiteralPath $candidate -Destination $install
    $installed=@(Get-Manifest $install)
    if (-not (Equal-Manifest $source $installed)) { throw 'Installed package does not match candidate' }
    Copy-Item -LiteralPath (Join-Path $install 'BUILD-INFO.json') -Destination (Join-Path $gameRoot 'BUILD-INFO.json')
    $afterPlayer=@(Get-PlayerManifest $playerRoot)
    if (-not (Equal-Manifest $afterPlayer @($before.player))) { throw 'Player data changed during deployment' }
    $result=[pscustomobject]@{passed=$true;version='0.2.0-preview.20261003.2';install=$install;backup=$backup;candidate=$candidate;source=$source;installed=$installed;old_backup_matches=$true;player_data_unchanged=$true;player_backup_matches=$true}
    Write-Json (Join-Path $evidence 'deploy-result.json') $result
    Write-Output ('Installed latest UI: {0} files match; original {1} files retained in complete program backup; {2} player/config files unchanged.' -f $installed.Count,$before.program.Count,$afterPlayer.Count)
} catch {
    if (Test-Path -LiteralPath $install) {
        Assert-Path $install 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\Windows'
        Assert-Path $failed 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\Windows-TASK053-latest-ui-failed'
        Move-Item -LiteralPath $install -Destination $failed
    }
    if (Test-Path -LiteralPath $oldProgram) {
        Assert-Path $oldProgram 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\备份-TASK053-20261003_latest_ui\Windows'
        Assert-Path $install 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2\Windows'
        Move-Item -LiteralPath $oldProgram -Destination $install
    }
    $priorInfo=Join-Path $backup 'RootFiles\BUILD-INFO.json'
    if (Test-Path -LiteralPath $priorInfo) { Copy-Item -LiteralPath $priorInfo -Destination (Join-Path $gameRoot 'BUILD-INFO.json') }
    Write-Json (Join-Path $evidence 'deploy-failure.json') @{passed=$false;error=$_.Exception.Message;restored_old_install=(Test-Path -LiteralPath $install)}
    throw
}
