import json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[3]))
from engine_adapters.ue5 import UEClient
root=Path(sys.argv[1])
profile=sys.argv[2] if len(sys.argv)>2 else "F:/HearthwardDemo/PlaytestProfile"
ue=UEClient(project_path=Path(__file__).resolve().parents[2]/"Hearthward.uproject",ue_root="G:/UnrealEngine/UE_5.8")
result=ue.runtime.launch_packaged(root/"Hearthward.exe",extra_args=("-windowed","-ResX=1280","-ResY=720","-HearthwardAIBackend=vulkan","-HearthwardAIGpuLayers=32",f"-UserDir={profile}"))
print(json.dumps(result,ensure_ascii=False),flush=True)
