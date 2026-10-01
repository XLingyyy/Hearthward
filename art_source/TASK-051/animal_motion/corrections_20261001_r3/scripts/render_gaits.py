"""Render real baked limb motion without modifying the deliverable scenes."""
import bpy
import sys
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import ROOT, REV, jobs, read, save, sha
from preview import setup

def encode(folder, files, fps, output, width, height, repeats=10):
    scene=bpy.data.scenes.new('R3Video')
    scene.render.resolution_x=width;scene.render.resolution_y=height;scene.render.resolution_percentage=100;scene.render.fps=fps
    scene.render.image_settings.file_format='FFMPEG';scene.render.ffmpeg.format='MPEG4';scene.render.ffmpeg.codec='H264';scene.render.ffmpeg.constant_rate_factor='HIGH';scene.render.use_sequencer=True
    strips=scene.sequence_editor_create().strips
    for i in range(repeats):
        strip=strips.new_image('cycle_'+str(i),filepath=str(folder/files[0]),channel=1,frame_start=1+i*len(files))
        for f in files[1:]:strip.elements.append(f)
    scene.frame_start=1;scene.frame_end=len(files)*repeats;scene.render.filepath=str(output)
    bpy.ops.render.render(animation=True,scene=scene.name)
    bpy.data.scenes.remove(scene)

def render(job, candidate=False, stills=False):
    slug=job['slug'];out=REV/'candidate'/slug if candidate else Path(job['output'])
    man=read(out/'animation_manifest.json');source=out/f'AS_{slug}.blend';digest=sha(source)
    bpy.ops.wm.open_mainfile(filepath=str(source))
    scene=bpy.context.scene;arm=next(o for o in scene.objects if o.type=='ARMATURE')
    length=man['rig']['axis'].get('body_reference_length_m',man['rig']['axis']['target_length_m'])
    height=max(v.co.z for o in scene.objects if o.type=='MESH' for v in o.data.vertices)
    cam,target=setup(arm,length)
    scene.eevee.taa_render_samples=16
    width,height_px=600,480;scene.render.resolution_x=width;scene.render.resolution_y=height_px
    scene.render.image_settings.file_format='PNG'
    records=[]
    selected=[c for c in man['clips'] if c['kind']=='run']
    if not stills:selected=[c for c in man['clips'] if c['kind'] in ('walk','hop','trot','run')]
    for c in selected:
        arm.animation_data.action=bpy.data.actions[c['name']]
        target=Vector((0,0,height*.43))
        # A slightly downward orthographic view keeps the large ground plane
        # from intersecting low view rays and hiding the actual feet.
        camera_z=target.z+length*.18
        for view,location in [('front',(length*2.3,0,camera_z)),('side',(0,-length*2.3,camera_z))]:
            # Closer framing makes joint and shaft quality inspectable.
            target=Vector((0,0,height*.43));cam.location=location
            cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
            cam.data.ortho_scale=max(length*(1.42 if view=='side' else .98),height*1.65)
            folder=out/'gait_preview_r3'/f'{c["suffix"]}_{view}';folder.mkdir(parents=True,exist_ok=True)
            fps=c['fps']
            times=[0,.25,.50,.75] if stills else [i/(c['frames']-1) for i in range(c['frames']-1)]
            files=[]
            for i,t in enumerate(times):
                frame=1+t*(c['frames']-1);scene.frame_set(int(frame),subframe=frame-int(frame))
                file=f'{i+1:05}.png';scene.render.filepath=str(folder/file)
                bpy.ops.render.render(write_still=True);files.append(file)
            if not stills:
                path=folder.parent/f'{folder.name}.mp4';encode(folder,files,fps,path,width,height_px,10)
                records.append({'action':c['name'],'suffix':c['suffix'],'kind':c['kind'],'view':view,'path':str(path),'fps':fps,'duration_s':c['duration']*10,'cycles':10,'source_blend_sha256':digest,'review_overlays_only':False})
            print('R3_RENDER',slug,c['suffix'],view,len(files),flush=True)
    save(out/'gait_preview_r3'/('still_index.json' if stills else 'index.json'),records)

if __name__=='__main__':
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    for j in jobs(args):render(j,'--candidate' in args,'--stills' in args)
