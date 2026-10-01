"""Render actual baked actions from the delivered editable source."""
import bpy,json,sys,math
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')

def setup(arm,length):
    scene=bpy.context.scene
    try:scene.render.engine='BLENDER_EEVEE'
    except TypeError:scene.render.engine='BLENDER_EEVEE_NEXT'
    scene.render.resolution_x=480;scene.render.resolution_y=360;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG';scene.render.film_transparent=False
    scene.world=bpy.data.worlds.new('QAWorld');scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.23,.26,.30,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.8
    for name,pos,power in [('Key',(1,-2,3),180),('Fill',(-2,1,1.6),90)]:
        data=bpy.data.lights.new(name,'AREA');data.energy=power*length*length;data.shape='DISK';data.size=2*length
        light=bpy.data.objects.new(name,data);scene.collection.objects.link(light);light.location=Vector(pos)*length
        light.rotation_euler=(-light.location+Vector((0,0,length*.3))).to_track_quat('-Z','Y').to_euler()
    data=bpy.data.cameras.new('QA');cam=bpy.data.objects.new('QA',data);scene.collection.objects.link(cam);scene.camera=cam
    height=max(v.co.z for o in scene.objects if o.type=='MESH' for v in o.data.vertices)
    cam.location=Vector((length*.75,-length*1.65,length*.9));target=Vector((0,0,height*.46))
    cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();data.type='ORTHO';data.ortho_scale=max(length*1.6,height*1.75)
    bpy.ops.mesh.primitive_plane_add(size=length*8,location=(0,0,-.001));floor=bpy.context.object
    mat=bpy.data.materials.new('QAFloor');mat.diffuse_color=(.17,.19,.22,1);floor.data.materials.append(mat)
    scene.view_settings.view_transform='AgX'
    return cam,target

def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    slug=args[0] if args else 'hare';requested=args[1:]
    job=next(j for j in json.loads((ROOT/'jobs.json').read_text(encoding='utf-8')) if j['slug']==slug)
    out=Path(job['output']);manifest=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'))
    bpy.ops.wm.open_mainfile(filepath=str(out/('AS_'+slug+'.blend')))
    arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    length=manifest['rig']['axis']['target_length_m'];cam,target=setup(arm,length)
    p=out/'previews';p.mkdir(exist_ok=True)
    clips=[c for c in manifest['clips'] if c['suffix'] in requested] if requested else [manifest['clips'][0],next(c for c in manifest['clips'] if c['kind'] in ('walk','hop','fish')),next(c for c in manifest['clips'] if c['kind'] in ('run','fish')),next(c for c in manifest['clips'] if c['kind'] in ('collapse','fish'))]
    for c in clips:
        arm.animation_data.action=bpy.data.actions[c['name']];bpy.context.scene.render.fps=c['fps']
        for frac in (.0,.25,.5,.75,1.0):
            frame=1+round((c['frames']-1)*frac);bpy.context.scene.frame_set(frame)
            bpy.context.scene.render.filepath=str(p/(c['suffix']+'_'+str(round(frac*100))+'.png'))
            bpy.ops.render.render(write_still=True)
        print('PREVIEW '+slug+' '+c['suffix'],flush=True)
    (p/'render_record.json').write_text(json.dumps({'source':str(out/('AS_'+slug+'.blend')),'resolution':[480,360],'render_engine':'EEVEE','actions':[c['name'] for c in clips]},indent=2),encoding='utf-8')

if __name__=='__main__':main()
