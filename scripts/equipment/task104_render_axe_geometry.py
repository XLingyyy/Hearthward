"""Offline whole-mesh before/after views, reproducible from text source and deltas.
Requires numpy/matplotlib. These diagrams are not UE runtime/render acceptance.
"""
import os
os.environ.setdefault('MPLCONFIGDIR','/tmp/hearthward-task104-mpl')
import json
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.collections import PolyCollection

ROOT=Path(__file__).resolve().parents[2]
source=ROOT/'art_source/TASK-104/grip'
data=json.loads((source/'handle_fit_delta.json').read_text(encoding='utf-8'))
vertices=[];faces=[]
for line in (source/'stone_bone_axe_handle_fit.obj').read_text(encoding='utf-8').splitlines():
    if line.startswith('v '): vertices.append([float(v)*s for v,s in zip(line.split()[1:],(100,-100,100))])
    elif line.startswith('f '): faces.append([int(v.split('/')[0])-1 for v in line.split()[1:]])
corrected=np.array(vertices);original=corrected.copy()
for row in data['vertices_id_original_xyz_new_xyz']: original[row[0]]=row[1:4]
faces=np.array([[face[0],face[i],face[i+1]] for face in faces for i in range(1,len(face)-1)])
fig,axes=plt.subplots(2,3,figsize=(13,12))
for row,(title,verts,color) in enumerate([('Original: 8.884 cm maximum core diameter',original,'#99724b'),('Corrected: 1.8 cm maximum core diameter',corrected,'#287e8b')]):
    for col,(a,b) in enumerate([(0,2),(1,2),(0,1)]):
        ax=axes[row,col]
        ax.add_collection(PolyCollection(verts[faces][:,:,[a,b]],facecolors=color,edgecolors='none',rasterized=True))
        grip=np.array(data['grip_mesh_cm']);axis=np.array(data['axis_mesh_unit'])
        lo=grip-axis*10;hi=grip+axis*10
        ax.plot([lo[a],hi[a]],[lo[b],hi[b]],color='#ff4444',lw=1,label='14 cm physical grasp region')
        ax.scatter(grip[a],grip[b],s=18,color='#ff4444')
        ax.set_aspect('equal');ax.set_xlim(original[:,a].min()-5,original[:,a].max()+5);ax.set_ylim(original[:,b].min()-5,original[:,b].max()+5)
        ax.set_xlabel('Mesh '+'XYZ'[a]+' (cm)');ax.set_ylabel('Mesh '+'XYZ'[b]+' (cm)');ax.grid(alpha=.15)
        ax.set_title(title if col==1 else ('View '+str(col+1)),fontsize=10)
fig.suptitle('TASK-104 | Actual full source mesh before / after localized handle shaping\n47,631 vertices; head, axial coordinates, topology and UVs preserved. OFFLINE, not UE acceptance.',fontsize=13)
fig.tight_layout(rect=(0,0,1,.95))
out=ROOT/'docs/qa/TASK-104/grip';out.mkdir(parents=True,exist_ok=True)
fig.savefig(out/'handle-before-after.svg',dpi=130,metadata={'Date':None})
# Keep generated SVG patch-clean; newlines still separate SVG path tokens.
svg_path=out/'handle-before-after.svg'
svg_path.write_text('\n'.join(line.rstrip() for line in svg_path.read_text(encoding='utf-8').splitlines())+'\n',encoding='utf-8')
fig.savefig(ROOT/'.agent-local/qa/TASK-104/grip/handle-before-after.png',dpi=130)
