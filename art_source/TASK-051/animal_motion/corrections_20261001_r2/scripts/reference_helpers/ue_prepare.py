"""Prepare an isolated UE 5.8.2 import-validation project through UEClient."""
from pathlib import Path
import json,os,sys,shutil
REPO=Path(__file__).resolve().parents[4];sys.path.insert(0,str(REPO))
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
os.environ['AAAGF_OUTPUT_ROOT']=r'E:\AiAgent\XLingGame\.animal-qa'
from pipeline.common.paths import task_output_dir
from engine_adapters.ue5 import UEClient

def main():
    project=task_output_dir('HW','pipeline','UEQA',run_id='qa')/'UEQA.uproject'
    client=UEClient(project_path=project,ue_root=r'E:\UE_5.8',port=30031,runtime_port=30032)
    if not project.is_file():
        result=client.project.create();(ROOT/'ue_project_create.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        if not result['ok']:raise RuntimeError(result['errors'])
    # Match the installed 5.8 toolchain and suppress unrelated optional previews.
    descriptor=json.loads(project.read_text(encoding='utf-8'));descriptor['EngineAssociation']='5.8'
    descriptor['Plugins']=[p for p in descriptor['Plugins'] if p['Name'] not in ('PixelStreaming','RemoteControlWebInterface','USDImporter')]
    native_plugin=REPO/'engine_adapters'/'ue5'/'plugin'/'A3GameAssetEditor'
    shutil.copytree(native_plugin, project.parent/'Plugins'/'A3GameAssetEditor', dirs_exist_ok=True)
    if not any(p['Name']=='A3GameAssetEditor' for p in descriptor['Plugins']):
        descriptor['Plugins'].append({'Name':'A3GameAssetEditor','Enabled':True})
    project.write_text(json.dumps(descriptor,indent=2),encoding='utf-8')
    for p in project.parent.glob('Source/*.Target.cs'):
        p.write_text(p.read_text(encoding='utf-8').replace('BuildSettingsVersion.V5','BuildSettingsVersion.V7'),encoding='utf-8')
    config=project.parent/'Config'/'DefaultEngine.ini'
    text=config.read_text(encoding='utf-8')
    if 'bRemoteExecution=True' not in text:text+='\n[/Script/PythonScriptPlugin.PythonScriptPluginSettings]\nbRemoteExecution=True\nRemoteExecutionMulticastGroupEndpoint=239.0.0.1:6766\nRemoteExecutionMulticastBindAddress=127.0.0.1\n\n[SystemSettings]\nr.DefaultFeature.AutoExposure=False\n'
    config.write_text(text,encoding='utf-8')
    result=client.build.project(target='UEQAEditor',timeout=1800)
    (ROOT/'ue_project_build.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print({'ok':result['ok'],'errors':result['errors']},flush=True)

if __name__=='__main__':main()
