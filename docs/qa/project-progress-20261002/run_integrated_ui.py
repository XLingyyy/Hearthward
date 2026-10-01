"""Exercise merged settings and save consent using a generated local test pool."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import sys
import time
import uuid

GAME=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(GAME/'scripts/animals'))
from paths import configure_factory,ue_root
configure_factory()
from engine_adapters.ue5 import UEClient


def save(path,value):
    path.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def main():
    evidence=Path(__file__).resolve().parent
    local=GAME/'Saved/UpdateCompatibility'
    local.mkdir(parents=True,exist_ok=True)
    pool=uuid.uuid4()
    source=max((GAME/'Saved/CompatibilityTests').glob('*/old.hws'),key=lambda p:p.stat().st_mtime)
    destination=GAME/'Saved/SaveGames/HearthwardPrototype'/('test-'+pool.hex.upper()+'.hws')
    destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(source,destination)
    save(local/'conflict-config.json',{'pool':str(destination)})
    client=UEClient(project_path=GAME/'Hearthward.uproject',ue_root=ue_root(),port=30031,runtime_port=30032)
    launch=client.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap',extra_args=[
        '-ExecutePythonScript='+str(evidence/'verify_integrated_ui.py'),
        '-HearthwardSaveTestPool='+str(pool),'-RenderOffScreen','-unattended','-NoSound','-NoLiveCoding','-NoSourceControl',
        '-abslog='+str(evidence/'integrated-ui.log')])
    save(evidence/'ui-launch.json',launch)
    assert launch['ok'],launch['errors']
    result_file=GAME/'Saved/ProjectProgress/IntegratedUI/results.json'
    assert not result_file.exists(),'Use a fresh verification output directory'
    try:
        for _ in range(180):
            if result_file.exists():
                result=json.loads(result_file.read_text('utf-8'))
                result['tested_commit']=subprocess.check_output(['git','rev-parse','HEAD'],cwd=GAME,text=True).strip()
                result['compiled_dll_sha256']=hashlib.sha256((GAME/'Binaries/Win64/UnrealEditor-Hearthward.dll').read_bytes()).hexdigest()
                save(evidence/'ui-result.json',result)
                screenshots=evidence/'ui-screenshots';screenshots.mkdir(exist_ok=True)
                for name in ['update-conflicts','update-conflicts-last','update-conflicts-resolved','progress-settings','progress-title']:
                    shutil.copy2(GAME/'Saved/Task020'/(name+'.png'),screenshots/(name+'.png'))
                print(json.dumps(result,ensure_ascii=False,indent=2),flush=True)
                assert result['ok'],result.get('error')
                return
            time.sleep(1)
        raise TimeoutError('Integrated UI verification did not finish')
    finally:
        save(evidence/'ui-stop.json',client.runtime.stop_editor(launch['payload']['process_id']))


if __name__=='__main__':
    main()
