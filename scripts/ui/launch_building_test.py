"""Preview building UI in a disposable new game; no Shipping package or desktop writes."""
import argparse, hashlib, json, os, shutil, sys, time, uuid
from pathlib import Path
GAME=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(GAME/'scripts/animals'))
from paths import configure_factory,ue_root
configure_factory()
from engine_adapters.ue5 import UEClient

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    run_id='hud_building_'+uuid.uuid4().hex[:12]
    evidence=GAME/'.agent-local/qa/TASK-076/building-preview'/run_id
    evidence.mkdir(parents=True)
    os.environ['HEARTHWARD_HUD_RUN']=run_id
    client=UEClient(project_path=GAME/'Hearthward.uproject',ue_root=ue_root(),port=30091,runtime_port=30092)
    preferences={p:p.read_bytes() for p in (GAME/'Saved/Config').glob('*/GameUserSettings.ini')}
    extra=['-HearthwardSaveTestPool='+str(uuid.uuid4()),'-abslog='+str(evidence/'runtime.log'),'-CoreLimit=4']
    if args.verify:
        extra+=['-RenderOffscreen','-unattended','-nosound','-ExecutePythonScript='+str(GAME/'.agent-local/qa/TASK-076/building-preview/verify_building.py')]
    else:
        extra+=['-game','-HearthwardHUDPreview','-windowed','-ResX=1600','-ResY=1000','-WinX=80','-WinY=80']
    launch=client.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap',extra_args=extra)
    (evidence/'launch.json').write_text(json.dumps(launch,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'run_id':run_id,'launch':launch},ensure_ascii=False,indent=2),flush=True)
    if not launch['ok']:raise SystemExit(1)
    files=['Resources/UI/interface.json','Resources/UI/layout.json','Source/Hearthward/UI/HearthwardScreenWidget.cpp','Source/Hearthward/UI/HearthwardScreenWidget.h','Source/Hearthward/UI/HearthwardScreenContent.cpp','Source/Hearthward/UI/HearthwardScreenActions.cpp','Source/Hearthward/UI/HearthwardScreenPaint.cpp','Resources/Data/gameplay.json','Binaries/Win64/UnrealEditor-Hearthward.dll']
    (evidence/'fingerprints.json').write_text(json.dumps({p:hashlib.sha256((GAME/p).read_bytes()).hexdigest() for p in files},indent=2),encoding='utf-8')
    if not args.verify:
        print('营地建造UI测试窗口已启动。加载后按B预览；仅使用临时存档池。',flush=True)
        return
    source=GAME/'Saved/HUDPreview'/run_id
    try:
        deadline=time.monotonic()+330
        while not (source/'report.json').is_file() and time.monotonic()<deadline:time.sleep(1)
        if not (source/'report.json').is_file():raise TimeoutError('Building verification did not finish')
        report=json.loads((source/'report.json').read_text(encoding='utf-8'))
        for p in source.glob('*'):
            if p.is_file():shutil.copy2(p,evidence/p.name)
        for p in report.get('ui_captures',[]):shutil.copy2(GAME/'Saved/Task020'/p,evidence/p)
        print(json.dumps({'passed':report['passed'],'checks':len(report['checks']),'error':report.get('error'),'evidence':str(evidence)},ensure_ascii=False,indent=2),flush=True)
        if not report['passed']:raise SystemExit(1)
    finally:
        stop=client.runtime.stop_editor(launch['payload']['process_id'])
        (evidence/'stop.json').write_text(json.dumps(stop,indent=2),encoding='utf-8')
        for p in set(preferences)|set((GAME/'Saved/Config').glob('*/GameUserSettings.ini')):
            if p in preferences:p.write_bytes(preferences[p])
            else:p.unlink()

if __name__=='__main__':main()
