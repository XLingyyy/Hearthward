"""Render true gait cycles and encode ten-repeat videos with Blender FFmpeg."""
import bpy,json,sys
from pathlib import Path
from mathutils import Vector
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
from preview import setup
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')

def encode(folder,frames,fps,output):
    scene=bpy.data.scenes.new('LoopVideo');scene.render.resolution_x=400;scene.render.resolution_y=300;scene.render.resolution_percentage=100
    scene.render.fps=fps;scene.render.image_settings.file_format='FFMPEG';scene.render.ffmpeg.format='MPEG4';scene.render.ffmpeg.codec='H264';scene.render.ffmpeg.constant_rate_factor='MEDIUM'
    scene.render.use_sequencer=True;seq=scene.sequence_editor_create();strips=seq.strips if hasattr(seq,'strips') else seq.sequences
    for cycle in range(10):
        strip=strips.new_image('cycle_'+str(cycle),filepath=str(folder/frames[0]),channel=1,frame_start=1+cycle*len(frames))
        for frame in frames[1:]:strip.elements.append(frame)
    scene.frame_start=1;scene.frame_end=10*len(frames);scene.render.filepath=str(output)
    bpy.ops.render.render(animation=True,scene=scene.name);bpy.data.scenes.remove(scene)

def main():
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    if args:jobs=[j for j in jobs if j['slug'] in args]
    for j in jobs:
        out=Path(j['output']);man=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'))
        bpy.ops.wm.open_mainfile(filepath=str(out/('AS_'+j['slug']+'.blend')));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
        cam,target=setup(arm,man['rig']['axis']['target_length_m']);scene=bpy.context.scene;scene.render.resolution_x=400;scene.render.resolution_y=300
        length=man['rig']['axis']['target_length_m'];floor=bpy.context.active_object
        floor.scale=(20,20,20)
        material=floor.active_material;material.use_nodes=True;nodes=material.node_tree.nodes;links=material.node_tree.links
        texture=nodes.new('ShaderNodeTexChecker');texture.inputs['Color1'].default_value=(.14,.16,.19,1);texture.inputs['Color2'].default_value=(.18,.21,.24,1);texture.inputs['Scale'].default_value=3/length
        coordinates=nodes.new('ShaderNodeTexCoord');links.new(coordinates.outputs['Object'],texture.inputs['Vector']);links.new(texture.outputs['Color'],nodes['Principled BSDF'].inputs['Base Color'])
        locomotion=bpy.data.objects.new('PreviewMovement',None);scene.collection.objects.link(locomotion)
        for obj in [arm,*[o for o in scene.objects if o.type=='MESH' and o!=floor]]:
            matrix=obj.matrix_world.copy();obj.parent=locomotion;obj.matrix_world=matrix
        camera_origin=cam.location.copy()
        if j['rig_type'] in ('aquatic','serpentine'):
            selected=[next(c for c in man['clips'] if c['suffix'] in ('SwimSlow','UndulateSlow')),next(c for c in man['clips'] if c['suffix'] in ('Burst','UndulateFast'))]
        else:selected=[next(c for c in man['clips'] if c['kind'] in ('walk','hop')),next(c for c in man['clips'] if c['kind']=='run')]
        records=[]
        for c in selected:
            arm.animation_data.action=bpy.data.actions[c['name']];scene.render.fps=c['fps']
            folder=out/'loop_preview'/c['suffix'];folder.mkdir(parents=True,exist_ok=True)
            frames=[]
            for f in range(1,c['frames']):
                movement=c.get('reference_speed_cm_s',0)/100*(f-1)/c['fps']
                locomotion.location.x=movement;cam.location.x=camera_origin.x+movement
                scene.frame_set(f);name=f'{f:04}.png';scene.render.filepath=str(folder/name);bpy.ops.render.render(write_still=True);frames.append(name)
            output=out/'loop_preview'/(c['suffix']+'_10loops.mp4');encode(folder,frames,c['fps'],output)
            records.append({'action':c['name'],'cycles':10,'native_fps':c['fps'],'cycle_seconds':c['duration'],'path':str(output)})
            print('VIDEO '+j['slug']+' '+c['suffix'],flush=True)
        (out/'loop_preview'/'index.json').write_text(json.dumps(records,indent=2),encoding='utf-8')

if __name__=='__main__':main()
