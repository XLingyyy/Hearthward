$ErrorActionPreference = 'Stop'
$gameRoot = 'C:\Users\22543\Desktop\Hearthward-20260929-9058ee2'
$evidence = 'E:\AiAgent\XLingGame\Hearthward\docs\qa\TASK-053\desktop_latest_20261003_preflight'
$record = Get-Content -LiteralPath (Join-Path $evidence 'before.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$actual = [IO.Path]::GetFullPath($record.backup)
$expected = [IO.Path]::GetFullPath((Join-Path $gameRoot '备份-TASK053-20261003_latest_ui'))
if (-not $actual.StartsWith($gameRoot+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Original backup escapes named installation root' }
if (-not $expected.StartsWith($gameRoot+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Corrected backup escapes named installation root' }
if ($actual -eq $expected) { throw 'Correction already complete; preserve evidence' }
if (Test-Path -LiteralPath $expected) { throw 'Do not overwrite another backup' }
if (-not (Test-Path -LiteralPath (Join-Path $actual 'Windows\BUILD-INFO.json'))) { throw 'Old complete backup identity missing' }
if ((Get-Item -LiteralPath $actual).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Original backup is a reparse point' }
if ((Get-Item -LiteralPath $gameRoot).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Installation root is a reparse point' }
# Resolved source and destination are explicitly checked immediately before this recursive move.
if ([IO.Path]::GetFullPath($actual) -ne $actual -or [IO.Path]::GetFullPath($expected) -ne $expected) { throw 'Final resolved path mismatch' }
Move-Item -LiteralPath $actual -Destination $expected
$result = @{passed=(Test-Path -LiteralPath (Join-Path $expected 'Windows\BUILD-INFO.json'));from=$actual;to=$expected;reason='Windows PowerShell 5.1 interpreted the UTF-8 script without BOM as ANSI; script files now use UTF-8 BOM.'}
[IO.File]::WriteAllText((Join-Path $evidence 'backup-path-correction.json'),($result|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
if (-not $result.passed) { throw 'Renamed backup identity missing' }
Write-Output 'Backup directory name corrected; program/player contents preserved.'
