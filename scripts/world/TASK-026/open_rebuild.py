"""Open the natural map with per-process DX12/SM6 settings via UEClient.

Run from GameFactory root with its Python environment. No project/global
configuration is changed. Use --game for an uncooked standalone browse.
"""
from pathlib import Path
import argparse
import json
import sys

project=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(project.parent))
from engine_adapters.ue5 import UEClient

parser=argparse.ArgumentParser()
parser.add_argument('--game',action='store_true')
parser.add_argument('--medium',action='store_true',help='Per-process Medium scalability, native 1080p; no saved config change')
parser.add_argument('--ue-root',default='G:/UnrealEngine/UE_5.8')
parser.add_argument('--grass-isolation',choices=['G0','G1','G2','G3'],help='Transient fresh-process grass source diagnostic')
args=parser.parse_args()
extra=['-d3d12','-sm6',
       '-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:D3D12TargetedShaderFormats=PCD3D_SM6']
if args.game:extra+=['-game','-windowed','-ResX=1920','-ResY=1080','-ForceRes']
commands=[]
if args.medium:
    names=['ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage','Shading','Landscape']
    commands=[f'sg.{name}Quality 1' for name in names]+['r.ScreenPercentage 100','r.DynamicRes.OperationMode 0','t.MaxFPS 0','r.VSync 0','stat fps']
if args.grass_isolation:
    if not args.game:parser.error('--grass-isolation requires --game')
    hism,grass={'G0':(0,0),'G1':(1,0),'G2':(0,1),'G3':(1,1)}[args.grass_isolation]
    commands += [f'ShowFlag.InstancedStaticMeshes {hism}',f'grass.Enable {grass}']
if commands:extra+=['-ExecCmds='+','.join(commands)]
client=UEClient(project_path=project/'Hearthward.uproject',ue_root=args.ue_root)
result=client.runtime.launch_editor(map_path='/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds',extra_args=extra)
print(json.dumps(result,ensure_ascii=False))
raise SystemExit(0 if result.get('ok') else 1)
