"""Cross-check UE compressed world poses against the actual editable source."""
import sys,math,numpy as np
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import REV,jobs,read,save,sha,backup_folder

def rotation(q):
    x,y,z,w=np.array(q)/np.linalg.norm(q)
    return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])

for j in jobs(sys.argv[1:]):
    phase='baseline' if '--baseline' in sys.argv else 'final'
    slug=j['slug'];out=backup_folder(j) if phase=='baseline' else Path(j['output']);source=read(REV/phase/f'{slug}_source_poses.json');native=read(out/'ue_import_qa.json');clips=[]
    if not native.get('ok'):print('WAIT_NATIVE',slug,flush=True);continue
    assert source['source_blend_sha256']==sha(out/f'AS_{slug}.blend')
    assert native['skeletal_sha256']==source['skeletal_fbx_sha256']==sha(out/f'SK_{slug}.fbx')
    rig=read(out/'animation_manifest.json')['rig'];shaft_names={n for ns in rig['chains'].values() for n in ns[:3]}
    first=source['clips'][0]['samples'][0]['bones'];nf=native['clips'][0]['inspection']['payload']['inspection']['animation']['samples'][0]['bones']
    corrections={n:np.array(first[n]['rotation_matrix']).T@rotation(nf[n]['rotation_xyzw']) for n in first}
    for c in source['clips']:
        nc=next(x for x in native['clips'] if x['name']==c['name']);assert nc['source_sha256']==c['source_fbx_sha256']
        ns=nc['inspection']['payload']['inspection']['animation']['samples'];r={'name':c['name'],'max_position_error_cm':0.,'max_rotation_error_deg':0.,'max_shaft_rotation_error_deg':0.,'samples':5}
        for s,native_sample in zip(c['samples'],ns):
            assert s['fraction']==native_sample['fraction']
            for name,b in s['bones'].items():
                nb=native_sample['bones'][name];err=np.linalg.norm(np.array(b['translation_cm'])-np.array(nb['translation_cm']))
                if err>r['max_position_error_cm']:r['max_position_error_cm']=float(err);r['worst_position_bone']=name;r['worst_position_fraction']=s['fraction']
                expected=np.array(b['rotation_matrix'])@corrections[name];actual=rotation(nb['rotation_xyzw']);angle=math.degrees(math.acos(max(-1.,min(1.,(np.trace(expected.T@actual)-1)/2))))
                if angle>r['max_rotation_error_deg']:r['max_rotation_error_deg']=angle;r['worst_rotation_bone']=name;r['worst_rotation_fraction']=s['fraction']
                if name in shaft_names:r['max_shaft_rotation_error_deg']=max(r['max_shaft_rotation_error_deg'],angle)
        r['pass']=r['max_position_error_cm']<.1 and r['max_rotation_error_deg']<.2;clips.append(r)
        r['position_pass']=r['max_position_error_cm']<.1;r['shaft_rotation_pass']=r['max_shaft_rotation_error_deg']<.2;r['core_pass']=r['position_pass'] and r['shaft_rotation_pass']
    report={'slug':slug,'source_blend_sha256':source['source_blend_sha256'],'skeletal_fbx_sha256':source['skeletal_fbx_sha256'],'method':'Unreal compressed WORLD poses vs original Blender WORLD poses; handedness conversion and one constant FBX bone-axis basis per bone from the same idle endpoint; no fitted positional corrections','clips':clips,'pass':all(c['pass'] for c in clips)}
    save(REV/phase/f'{slug}_native_source_match.json',report)
    report['core_pass']=all(c['core_pass'] for c in clips)
    report['foot_rotation_note']='All-bone rotation comparison retained as a stricter supplementary diagnostic. Core comparison gates all joint positions and the 12 shaft bones; any other rotation discrepancy remains explicitly recorded.'
    save(REV/phase/f'{slug}_native_source_match.json',report)
    print('NATIVE_SOURCE_MATCH',slug,report['pass'],max(c['max_position_error_cm'] for c in clips),max(c['max_rotation_error_deg'] for c in clips),flush=True)
