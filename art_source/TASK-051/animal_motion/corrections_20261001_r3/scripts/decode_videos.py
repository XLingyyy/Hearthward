"""Reopen delivered MP4 files and verify actual frames, rate and duration."""
import bpy,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import ROOT,REV,SLUGS,read,save,sha
records=[];prior=read(REV/'video_encoding_qa.json') if (REV/'video_encoding_qa.json').is_file() else []
for j in read(ROOT/'jobs.json'):
    out=Path(j['output']);changed=j['slug'] in SLUGS
    index=out/('gait_preview_r3' if changed else 'loop_preview')/'index.json'
    for v in read(index):
        fps=v.get('fps',v.get('native_fps'));duration=v['duration_s'] if changed else v['cycles']*v['cycle_seconds'];path=Path(v['path'])
        assert v['source_blend_sha256']==sha(out/f'AS_{j["slug"]}.blend'),j['slug']+' stale preview'
        old=next((r for r in prior if r['path']==str(path) and r['source_blend_sha256']==v['source_blend_sha256'] and r['sha256']==sha(path) and r['pass']),None)
        if old is not None:
            records.append(old);continue
        previous=bpy.context.window.scene;scene=bpy.data.scenes.new('DecodeR3');bpy.context.window.scene=scene;scene.render.fps=fps;scene.render.use_sequencer=True;scene.render.resolution_x=400;scene.render.resolution_y=320;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG'
        movie=scene.sequence_editor_create().strips.new_movie('gait',filepath=str(path),channel=1,frame_start=1);n=movie.frame_duration
        assert abs(movie.fps-fps)<.01,(path,movie.fps,fps)
        assert abs(n/fps-duration)<1.1/fps,(path,n/fps,duration)
        folder=REV/'video_decode'/j['slug']/path.stem;folder.mkdir(parents=True,exist_ok=True);samples=[];cycle=round(duration/v['cycles']*fps)
        for fraction in (0.,.25,.5,.75):
            frame=1+round((cycle-1)*fraction);scene.frame_set(frame);target=folder/f'{round(fraction*100)}.png';scene.render.filepath=str(target);bpy.ops.render.render(write_still=True,scene=scene.name);samples.append({'frame':frame,'path':str(target)})
        records.append({'slug':j['slug'],'action':v['action'],'view':v.get('view','oblique'),'path':str(path),'sha256':sha(path),'source_blend_sha256':v['source_blend_sha256'],'fps':movie.fps,'decoded_frames':n,'duration_s':n/fps,'expected_duration_s':duration,'samples':samples,'pass':True})
        bpy.context.window.scene=previous;bpy.data.scenes.remove(scene);print('VIDEO_DECODE',j['slug'],path.stem,n,fps,flush=True)
    save(REV/'video_encoding_qa.json',records)
assert len([r for r in records if r['slug'] in SLUGS])==46
