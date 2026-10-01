"""Blender anatomical/weight audit; never infers anatomy from bone IDs alone."""
import bpy,json,sys,math
from pathlib import Path
from mathutils import Vector

ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')

def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    if args:jobs=[j for j in jobs if j['slug'] in args]
    for job in jobs:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=job['source'],use_image_search=False)
        arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
        meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
        points=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
        mn=[min(p[i] for p in points) for i in range(3)];mx=[max(p[i] for p in points) for i in range(3)]
        totals={b.name:[0.0,Vector(),0,Vector((1e9,1e9,1e9)),Vector((-1e9,-1e9,-1e9))] for b in arm.data.bones}
        unweighted=0;max_influences=0;sum_error=0.0
        for o in meshes:
            names={g.index:g.name for g in o.vertex_groups}
            for v in o.data.vertices:
                influences=[g for g in v.groups if names[g.group] in totals and g.weight>1e-5]
                max_influences=max(max_influences,len(influences))
                if not influences:unweighted+=1
                sum_error=max(sum_error,abs(1-sum(g.weight for g in influences)))
                p=o.matrix_world@v.co
                for g in influences:
                    t=totals[names[g.group]];t[0]+=g.weight;t[1]+=p*g.weight;t[2]+=1
                    for i in range(3):t[3][i]=min(t[3][i],p[i]);t[4][i]=max(t[4][i],p[i])
        bones=[]
        for b in arm.data.bones:
            t=totals[b.name]
            bones.append({'name':b.name,'parent':b.parent.name if b.parent else None,
             'head':list(arm.matrix_world@b.head_local),'tail':list(arm.matrix_world@b.tail_local),
             'weight_sum':t[0],'weighted_vertices':t[2],'weight_center':list(t[1]/t[0]) if t[0]>0 else None,
             'weight_bounds':[list(t[3]),list(t[4])] if t[2]>0 else None})
        out=Path(job['output'])
        report={'slug':job['slug'],'bounds':[mn,mx],'dimensions':[mx[i]-mn[i] for i in range(3)],
         'armature_matrix':[list(row) for row in arm.matrix_world], 'armature':arm.name,
         'vertices':len(points),'bones':bones,'unweighted_vertices':unweighted,'weight_sum_max_error':sum_error,
         'max_influences':max_influences,'images':[{'name':i.name,'size':list(i.size),'packed':bool(i.packed_file),'filepath':i.filepath} for i in bpy.data.images]}
        (out/'rig_audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        bpy.ops.file.pack_all()
        bpy.ops.wm.save_as_mainfile(filepath=str(out/'source_import.blend'))
        print('AUDIT '+job['slug']+' bones '+str(len(bones))+' dimensions '+str(report['dimensions']),flush=True)

if __name__=='__main__':main()
