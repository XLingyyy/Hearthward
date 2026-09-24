"""Read-only diagnosis of animation travel relative to the character capsule."""
import json, traceback
from pathlib import Path
import unreal

report={'ok':False,'clips':{}}
try:
    for character, folder in [('Hero','AnimationV2'),('Brother','Animation')]:
        for name in ['Idle','Walk','Run','Dig','Attack']:
            seq=unreal.load_asset(f'/Game/Characters/{character}/{folder}/A_{character}_{name}')
            tracks={}
            for bone in ['root','pelvis']:
                values=[unreal.AnimationLibrary.get_bone_pose_for_time(seq,bone,seq.get_play_length()*i/24,False) for i in range(25)]
                tracks[bone]={'translation_samples':[[p.translation.x,p.translation.y,p.translation.z] for p in values],
                              'scale':str(values[0].scale3d),'rotation':str(values[0].rotation)}
            report['clips'][character+'_'+name]={'length':seq.get_play_length(),'force_root_lock':seq.get_editor_property('force_root_lock'),'tracks':tracks}
    report['ok']=True
except Exception: report['error']=traceback.format_exc()
(Path(unreal.Paths.project_saved_dir())/'HeroValidation/root-audit-before.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
