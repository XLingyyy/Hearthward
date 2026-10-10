"""Blender source rig extraction; read-only exact originals, local QA output only."""
import bpy,json
from pathlib import Path
import hashlib, shutil
ROOT=Path(__file__).resolve().parents[2]
out=ROOT/'.agent-local/qa/TASK-104/grip'
out.mkdir(parents=True,exist_ok=True)
for role in ('Hero','Brother'):
 source=ROOT/'art_source/TASK-095/source-cache'/role/('SK_'+role+'.fbx')
 raw=source.read_bytes()
 if raw.startswith(b'version https://git-lfs.github.com/spec/v1'):
  sha=raw.decode().split('sha256:')[1].split()[0]
  source=ROOT/'.git/lfs/objects'/sha[:2]/sha[2:4]/sha
  assert source.exists(), 'Fetch the exact source FBX via git lfs first'
 else: sha=hashlib.sha256(raw).hexdigest()
 assert sha=={'Hero':'c28eefcb8b209c62d5c97bea3895ca2daf56eacbf4e5fb353b71fe6ada374046','Brother':'696ad97e53a638cefd208dedf064b964afe61a2c0c40c484240a593629947a47'}[role]
 shutil.copyfile(source,out/(role+'.fbx'))
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(out/(role+'.fbx')),use_anim=False)
 arms=[o for o in bpy.context.scene.objects if o.type=='ARMATURE']
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
 print(role,[(o.name,len(o.data.bones)) for o in arms],[(o.name,len(o.data.vertices)) for o in meshes])
 a=arms[0]
 data={'armature_matrix':[list(r) for r in a.matrix_world],'bones':{b.name:{'matrix':[list(r) for r in b.matrix_local],'parent':b.parent.name if b.parent else None} for b in a.data.bones},'meshes':[]}
 for o in meshes:
  data['meshes'].append({'matrix':[list(r) for r in o.matrix_world],'vertices':[list(o.matrix_world@v.co) for v in o.data.vertices],'faces':[list(p.vertices) for p in o.data.polygons],'weights':[[[o.vertex_groups[g.group].name,g.weight] for g in v.groups] for v in o.data.vertices]})
 (out/(role+'-rig.json')).write_text(json.dumps(data))
