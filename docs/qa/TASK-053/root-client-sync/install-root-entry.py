"""Create UTF-8/CRLF Windows entry points without shell quote interpolation."""
import json
from pathlib import Path

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
ignore = (GAME/'TestClient/.gitignore').read_bytes()
(QA/'before/TestClient__.gitignore.snapshot').write_bytes(ignore.replace(b'/Backups/\r\n',b'').replace(b'/Backups/\n',b''))
source = (QA/'player-backup').resolve()
destination = (GAME/'TestClient/Backups/20261004-root-client-sync').resolve()
assert source.is_relative_to(GAME) and destination.is_relative_to(GAME/'TestClient')
assert not destination.exists()
destination.parent.mkdir(parents=True,exist_ok=True)
source.rename(destination)
context = json.loads((QA/'context.json').read_text(encoding='utf-8'))
context.update(snapshot_count=7,player_backup=destination.as_posix())
(QA/'context.json').write_text(json.dumps(context,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

def cmd(path,text):
    path.write_bytes(text.replace('\n','\r\n').encode('utf-8'))

cmd(GAME/'启动测试版游戏.cmd',r'''@echo off
chcp 65001 >nul
call "%~dp0TestClient\启动测试端.cmd" %*
''')
cmd(GAME.parent/'启动测试版游戏.cmd',r'''@echo off
chcp 65001 >nul
call "%~dp0Hearthward\启动测试版游戏.cmd" %*
''')
cmd(GAME/'TestClient/启动测试端.cmd',r'''@echo off
chcp 65001 >nul
cd /d "%~dp0.."
set "TEST_CLIENT_PYTHON=python"
if exist "E:\RealPython\python.exe" set "TEST_CLIENT_PYTHON=E:\RealPython\python.exe"
if defined HEARTHWARD_PYTHON set "TEST_CLIENT_PYTHON=%HEARTHWARD_PYTHON%"
"%TEST_CLIENT_PYTHON%" -X utf8 "%~dp0..\scripts\ui\launch_test_client.py" %*
if errorlevel 1 if "%~1"=="" pause
''')
print('Root game entry and persistent client updated; profile backup stored in ignored TestClient/Backups.')
