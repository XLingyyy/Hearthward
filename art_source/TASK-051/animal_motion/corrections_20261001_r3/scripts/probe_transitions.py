import bpy,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import jobs,read
from audit_refinement import pose,angle
from common import REV
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else ['stag_a','black_bear']
for j in jobs(args):
    out=REV/'candidate'/j['slug'] if '--candidate' in args else Path(j['output']);m=read(out/'animation_manifest.json');bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{j["slug"]}.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    for c in m['clips']:
      if c['kind'] not in ('start','stop','run'):continue
      arm.animation_data.action=bpy.data.actions[c['name']];last=None;worst=(0,None,None)
      for f in range(1,c['frames']+1):
        bpy.context.scene.frame_set(f);p=pose(arm)
        if last:
          a,n=max((angle(last[n][1],p[n][1]),n) for n in p)
          if a>worst[0]:worst=(a,n,f)
        last=p
      print('WORST',j['slug'],c['suffix'],worst,flush=True)
      for f in range(max(1,worst[2]-1),min(c['frames'],worst[2]+1)+1):
        bpy.context.scene.frame_set(f);print('POINTS',f,{k:[list(arm.pose.bones[n].matrix.translation) for n in ns[:4]] for k,ns in m['rig']['chains'].items()},flush=True)
