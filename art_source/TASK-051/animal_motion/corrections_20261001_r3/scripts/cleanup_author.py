"""Remove exploratory implementations from the reproducible delivery script."""
from pathlib import Path
p=Path(__file__).with_name('correct_gait.py')
s=p.read_text(encoding='utf-8')
if 'def stiffen_skin(' in s:s=s[:s.index('def stiffen_skin(')]+s[s.index('def author('):]
s=s.replace('from pose_tools import set_world\n','')
s=s.replace('Existing body motion, foot tracks,','Authored foot tracks,')
s=s.replace('skeleton names/rest matrices, vertices, UVs and textures stay\npaired. Only limb gait curves and spatially verified limb weights are edited.','skeleton names, vertices, UVs and textures stay paired. Limb rest pivots are\nmeasured and all paired actions retargeted; gait curves and shaft weights are edited.')
s=s.replace("rig['repairs'].append('2026-10-01 R3: rigid limb shaft weights and compact adjacent joint blending; medial sagittal gait solve at exact segment lengths')", "rig['repairs'].append('2026-10-01 R3: measured limb rest pivots; retargeted complete library; calibrated shaft skinning; exact-length medial gait solve and restrained torso compression')")
lines=s.splitlines();result=[];inside=False
for line in lines:
    if line=='        if True:':inside=True;continue
    if inside and line.startswith('        # SK skin'):inside=False
    if inside and line.startswith('    '):line=line[4:]
    result.append(line)
p.write_text('\n'.join(result)+'\n',encoding='utf-8')
print('Author script retains the verified rest fitting, retargeting and rigid gait solver')
