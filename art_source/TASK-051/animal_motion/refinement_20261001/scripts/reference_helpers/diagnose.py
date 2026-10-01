"""Locate deforming skin contact and strain failures on authored meshes."""
import bpy,json,sys
from pathlib import Path
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
def main():
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    for slug,suffix in [('black_bear','Lope'),('red_fox','RunFlee'),('goat','Rest'),('hare','Nibble_Loop'),('wolf','Gallop')]:
        job=next(j for j in jobs if j['slug']==slug);out=Path(job['output']);m=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'))
        bpy.ops.wm.open_mainfile(filepath=str(out/('AS_'+slug+'.blend')));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
        c=next(c for c in m['clips'] if c['suffix']==suffix);arm.animation_data.action=bpy.data.actions[c['name']];bpy.context.scene.frame_set(1);bpy.context.view_layer.update()
        dg=bpy.context.evaluated_depsgraph_get()
        print('DIAG',slug,suffix,'feet',m['rig']['restfeet'],flush=True)
        for obj in meshes:
            ev=obj.evaluated_get(dg);mesh=ev.to_mesh();vs=sorted(mesh.vertices,key=lambda v:v.co.z)
            for v in vs[:3]:
                original=obj.data.vertices[v.index];groups={obj.vertex_groups[g.group].name:round(g.weight,3) for g in original.groups}
                print('GROUND',v.index,list(v.co),list(original.co),groups,flush=True)
            edges=[]
            for e in obj.data.edges:
                a,b=e.vertices;length=(obj.data.vertices[a].co-obj.data.vertices[b].co).length
                if length>.0003:edges.append(((mesh.vertices[a].co-mesh.vertices[b].co).length/length,a,b))
            for ratio,a,b in sorted(edges,reverse=True)[:5]:
                print('STRAIN',round(ratio,2),list(obj.data.vertices[a].co),{obj.vertex_groups[g.group].name:round(g.weight,3) for g in obj.data.vertices[a].groups},{obj.vertex_groups[g.group].name:round(g.weight,3) for g in obj.data.vertices[b].groups},flush=True)
            ev.to_mesh_clear()
if __name__=='__main__':main()
