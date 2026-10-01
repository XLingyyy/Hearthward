import bpy,json,math
from pathlib import Path
root=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
out=root/'Hearthward/animal_motion_20260930/assets/motion/goat';m=json.loads((out/'animation_manifest.json').read_text('utf-8'));bpy.ops.wm.open_mainfile(filepath=str(out/'AS_goat.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
for s in ['LieDown','GetUp']:
 c=next(c for c in m['clips'] if c['suffix']==s);arm.animation_data.action=bpy.data.actions[c['name']];prev=None;changes=[]
 for f in range(1,c['frames']+1):
  bpy.context.scene.frame_set(f);p={b.name:(b.matrix.to_quaternion(),b.matrix.translation.copy()) for b in arm.pose.bones}
  if prev:
   for n,(q,v) in p.items():
    a=math.degrees(prev[n][0].rotation_difference(q).angle)
    if a>40:changes.append((f,n,round(a,2),round((v-prev[n][1]).length*100,2)))
  prev=p
 print('GOAT_JUMP',s,changes,flush=True)

