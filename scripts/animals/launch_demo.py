"""Launch the local playable animal exhibit, optionally run native verification."""
import sys,json,time,argparse,uuid,shutil,subprocess,hashlib
from pathlib import Path
GAME=Path(__file__).resolve().parents[2]
from paths import configure_factory,ue_root
configure_factory()
from engine_adapters.ue5 import UEClient
OUT=GAME/'docs/qa/TASK-051'
def save(p,v):Path(p).write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report-dir',type=Path,default=OUT);args=parser.parse_args()
    output=args.report_dir;output.mkdir(parents=True,exist_ok=True)
    report=json.loads((OUT/'world_report.json').read_text('utf-8'));assert report['ok']
    level=report['world']['payload']['world']['metadata']['level_path']
    client=UEClient(project_path=GAME/'Hearthward.uproject',ue_root=ue_root(),port=30031,runtime_port=30032)
    run=uuid.uuid4().hex[:12];evidence=output/('runtime_'+run if args.verify else 'interactive_'+run)
    evidence.mkdir(parents=True,exist_ok=True)
    extra=['-game','-windowed','-ResX=1600','-ResY=900','-NoSound','-NoLiveCoding','-NoSourceControl','-nosplash','-HearthwardSaveTestPool=animal_demo_'+run,'-abslog='+str(evidence/'game.log')]
    if args.verify:
        extra.extend(['-HearthwardAnimalDemoVerify','-RenderOffScreen','-unattended','-UseFixedTimeStep','-FPS=60'])
        saved_root=(GAME/'Saved').resolve()
        previous=(saved_root/'AnimalDemo').resolve()
        archive=(saved_root/('AnimalDemo_previous_'+run)).resolve()
        if previous.exists():
            if not previous.is_relative_to(saved_root) or not archive.is_relative_to(saved_root):
                raise RuntimeError('Verification archive paths must stay inside this game Saved directory')
            if previous == saved_root or archive == saved_root or archive.exists():
                raise RuntimeError('Refusing an ambiguous verification archive target')
            shutil.move(str(previous),str(archive))
    files=list((GAME/'Source/Hearthward').rglob('*'))+[GAME/'Resources/Data/animal_motion.json',GAME/'Resources/Data/gameplay.json',GAME/'Config/DefaultEngine.ini',GAME/'Hearthward.uproject']
    binding={'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=GAME,text=True).strip(),'branch':subprocess.check_output(['git','branch','--show-current'],cwd=GAME,text=True).strip(),'source_sha256':{str(p.relative_to(GAME)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in files if p.is_file()},'map_sha256':hashlib.sha256((GAME/'Content'/Path(level.removeprefix('/Game/')+'.umap')).read_bytes()).hexdigest(),'compiled_dll_sha256':hashlib.sha256((GAME/'Binaries/Win64/UnrealEditor-Hearthward.dll').read_bytes()).hexdigest()}
    asset_roots=[GAME/'Content/Hearthward/Animals',GAME/'Content/Hearthward/Tests/AnimalMotion',GAME/'Content/Imported/Scenes/animal_demo_20261001']
    binding['asset_sha256']={str(p.relative_to(GAME)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for root in asset_roots for p in root.rglob('*') if p.is_file()}
    save(evidence/'binding.json',binding)
    launch=client.runtime.launch_editor(map_path=level+'?game=/Script/Hearthward.HearthwardAnimalDemoGameMode',extra_args=extra)
    save(evidence/'launch.json',launch);save(output/'last_launch.json',{'evidence':str(evidence),'verify':args.verify,**launch});assert launch['ok'],launch
    print('ANIMAL_GAME_LAUNCHED',launch['payload']['process_id'],str(evidence),flush=True)
    if not args.verify:return
    try:
        result_file=GAME/'Saved/AnimalDemo/verification.json'
        for second in range(600):
            if (evidence/'stop_requested').exists():raise RuntimeError('Native verification stopped by the host request')
            if result_file.exists():
                result=json.loads(result_file.read_text('utf-8'));shutil.copytree(GAME/'Saved/AnimalDemo',evidence/'native',dirs_exist_ok=True)
                save(output/'runtime_result.json',{'evidence':str(evidence),'binding':binding,'native':result,'ok':result['pass']})
                print('NATIVE_ANIMAL_CHECKS',len(result['checks']),result['pass'],flush=True)
                if not result['pass']:raise RuntimeError('Native animal checks failed; inspect runtime_result.json')
                return
            if second%30==0:print('VERIFY_RUNNING',second,flush=True)
            time.sleep(1)
        raise TimeoutError('Native animal verification did not finish within 600 seconds')
    finally:save(evidence/'stop.json',client.runtime.stop_editor(launch['payload']['process_id']))
if __name__=='__main__':main()
