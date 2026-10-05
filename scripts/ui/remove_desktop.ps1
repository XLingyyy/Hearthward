param([switch]$Execute)
$ErrorActionPreference = 'Stop'
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$install = 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2'
$profile = 'C:\Users\22543\AppData\Local\Hearthward'
$backup = Join-Path $project 'Saved\DesktopRemoval\20261003'
$evidence = Join-Path $project 'docs\qa\TASK-053\input-lifecycle-fix'

function Assert-ExactRoot([string]$Path) {
    $resolved = (Resolve-Path -LiteralPath $Path -ErrorAction Stop).Path
    if ($resolved -ne $Path) { throw "Unexpected removal target: $resolved" }
    $entries = @((Get-Item -LiteralPath $resolved -Force)) + @(Get-ChildItem -LiteralPath $resolved -Recurse -Force)
    if (@($entries | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }).Count) {
        throw "Refusing removal with reparse points: $resolved"
    }
    return $resolved
}

$targets = @($install, $profile) | Where-Object { Test-Path -LiteralPath $_ }
$targets = @($targets | ForEach-Object { Assert-ExactRoot $_ })
if (-not (Test-Path -LiteralPath (Join-Path $install 'Windows\BUILD-INFO.json'))) {
    throw 'Recognized desktop installation metadata is required.'
}
$running = @(Get-CimInstance Win32_Process | Where-Object {
    $_.ExecutablePath -and $_.ExecutablePath.StartsWith($install + '\', [StringComparison]::OrdinalIgnoreCase)
})
if ($running.Count) { throw 'A process is using this installation; preserve it until closed.' }
$beforeFree = (Get-PSDrive C).Free
$inventory = @($targets | ForEach-Object {
    $root = $_
    $files = @(Get-ChildItem -LiteralPath $root -File -Recurse -Force)
    [pscustomobject]@{root=$root; files=$files.Count; bytes=($files | Measure-Object Length -Sum).Sum}
})
if (-not $Execute) { $inventory | ConvertTo-Json -Depth 4; return }
if (Test-Path -LiteralPath $backup) { throw 'Existing backup must not be overwritten.' }
New-Item -ItemType Directory -Path $backup, $evidence -Force | Out-Null

# Keep every live player file and historical saves/settings; program/model copies can be removed.
$preserved = @()
foreach ($root in $targets) {
    $label = if ($root -eq $profile) { 'PlayerProfile' } else { 'DesktopMetadataAndPlayerBackups' }
    foreach ($file in Get-ChildItem -LiteralPath $root -File -Recurse -Force) {
        $relative = $file.FullName.Substring($root.Length + 1)
        if ($root -eq $install -and $relative -notmatch '(?i)(^|\\)(PlayerSaved|SaveGames|RootFiles)(\\|$)|\.(sav|ini|json|txt|cmd)$') { continue }
        $destination = Join-Path (Join-Path $backup $label) $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $destination
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
        if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $hash) {
            throw "Player backup differs: $relative"
        }
        $preserved += [pscustomobject]@{source=$file.FullName; backup=$destination; bytes=$file.Length; sha256=$hash}
    }
}

$shortcutFiles = @()
$shortcutRoots = @([Environment]::GetFolderPath('Desktop'), 'C:\Users\Public\Desktop',
    'C:\Users\22543\AppData\Roaming\Microsoft\Windows\Start Menu\Programs') | Select-Object -Unique
$shellLinks = New-Object -ComObject WScript.Shell
foreach ($root in $shortcutRoots) {
    if (-not (Test-Path -LiteralPath $root)) { continue }
    foreach ($file in Get-ChildItem -LiteralPath $root -File -Filter '*.lnk' -Recurse -Force) {
        $link = $shellLinks.CreateShortcut($file.FullName)
        if ($link.TargetPath.StartsWith($install + '\', [StringComparison]::OrdinalIgnoreCase)) {
            $destination = Join-Path $backup ('Shortcuts\' + $file.Name)
            New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
            Copy-Item -LiteralPath $file.FullName -Destination $destination
            if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) { throw 'Shortcut backup differs.' }
            $shortcutFiles += $file.FullName
        }
    }
}

$result = [ordered]@{date='2026-10-03'; authorized_by='User request: remove C-drive desktop Hearthward; all future tests in project';
    inventory=$inventory; player_backup=$backup; preserved=$preserved; removed_shortcuts=$shortcutFiles;
    c_free_before=$beforeFree; completed=$false}
$encoding = New-Object Text.UTF8Encoding($false)
$report = Join-Path $evidence 'desktop-removal.json'
[IO.File]::WriteAllText($report, ($result | ConvertTo-Json -Depth 8), $encoding)
foreach ($root in $targets) {
    # Recheck the absolute target immediately before each recursive removal, in this same shell.
    $checked = Assert-ExactRoot $root
    if ($checked -ne $install -and $checked -ne $profile) { throw 'Removal escaped the authorized game directories.' }
    Remove-Item -LiteralPath $checked -Recurse -Force
}
foreach ($shortcut in $shortcutFiles) { Remove-Item -LiteralPath $shortcut -Force }
$result.completed = -not (Test-Path -LiteralPath $install) -and -not (Test-Path -LiteralPath $profile)
$result.c_free_after = (Get-PSDrive C).Free
$result.c_free_recovered = $result.c_free_after - $beforeFree
[IO.File]::WriteAllText($report, ($result | ConvertTo-Json -Depth 8), $encoding)
[pscustomobject]@{completed=$result.completed; removed_bytes=($inventory | Measure-Object bytes -Sum).Sum;
    c_free_recovered=$result.c_free_recovered; c_free_after=$result.c_free_after; player_files_preserved=$preserved.Count;
    backup=$backup; report=$report} | ConvertTo-Json -Compress
