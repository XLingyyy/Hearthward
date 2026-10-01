"""Audit editable animal motion before/after refinement. Run in Blender 4.5.

Reports actual baked poses, explicit transition endpoints, cumulative stance
sliding, loop tangent changes, skin strain, fixed holds and spatial limb roles.
This audit never treats an FBX import as a gameplay or visual-quality pass.
"""
import bpy
import hashlib
import json
import math
import sys
from pathlib import Path
import numpy as np

ROOT = Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
REV = ROOT / 'corrections_20261001_r2'
sys.path.insert(0, str(Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果\corrections_20261001_r2/scripts/reference_helpers')))
from author import curves


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def angle(a, b):
    q = a.rotation_difference(b)
    return math.degrees(2 * math.acos(min(1.0, abs(q.w))))


def pose(arm):
    return {p.name: (p.matrix.translation.copy(), p.matrix.to_quaternion()) for p in arm.pose.bones}


def difference(a, b):
    return {'position_cm': max((a[n][0] - b[n][0]).length * 100 for n in a),
            'rotation_deg': max(angle(a[n][1], b[n][1]) for n in a)}


def points(meshes):
    dg = bpy.context.evaluated_depsgraph_get()
    result = []
    for obj in meshes:
        evaluated = obj.evaluated_get(dg)
        mesh = evaluated.to_mesh()
        xyz = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
        mesh.vertices.foreach_get('co', xyz)
        xyz = xyz.reshape((-1, 3))
        transform = np.array(evaluated.matrix_world)
        result.append(xyz @ transform[:3, :3].T + transform[:3, 3])
        evaluated.to_mesh_clear()
    return np.concatenate(result)


def transitions(man):
    suffixes = {c['suffix'] for c in man['clips']}
    pairs = []
    for stem in ('Graze', 'Nibble', 'Eat', 'DustBath', 'RearSniff'):
        middle = stem + ('_Hold' if stem == 'RearSniff' else '_Loop')
        if stem + '_In' in suffixes:
            pairs += [(stem + '_In', middle), (middle, stem + '_Out')]
    for begin, rest, end in [('LieDown', 'Rest', 'GetUp'), ('CurlDown', 'CurlRest', 'GetUp'),
                              ('NestSettle', 'NestRest', 'NestExit'),
                              ('Takeoff', 'FlightShort', 'Land')]:
        if begin in suffixes:
            pairs += [(begin, rest), (rest, end)]
    for side in ('L', 'R'):
        if 'Collapse_' + side in suffixes:
            pairs.append(('Collapse_' + side, 'CorpseHold_' + side))
    for settle in ('Settle', 'SettleArc'):
        if settle in suffixes:
            pairs.append((settle, 'DisplayStill'))
    pairs += [('Hooked_In','Struggle'),('Hooked_In','StruggleS'),
              ('LiftOut','LandFlop'),('LiftSupported','LandWriggle'),
              ('LandFlop','Settle'),('LandWriggle','SettleArc'),
              ('Brake','Hover'),('Brake','HoverLow'),('Burst','Coast'),('Burst','PauseGlide'),
              ('HoverWave','StartWave'),('StartWave','UndulateCruise'),
              ('UndulateCruise','StopWave'),('StopWave','HoverWave')]
    return [(a, b) for a, b in pairs if a in suffixes and b in suffixes]


def audit(job,phase):
    out = Path(job['output'])
    source_folder = REV/'backup'/out.relative_to(ROOT.parent) if phase=='baseline' else out
    man = json.loads((source_folder / 'animation_manifest.json').read_text('utf-8'))
    source = source_folder / f"AS_{job['slug']}.blend"
    bpy.ops.wm.open_mainfile(filepath=str(source))
    arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.name.startswith('BindPoseProxy')]
    scene = bpy.context.scene
    arm.animation_data.action = None
    for p in arm.pose.bones:
        p.location = (0, 0, 0)
        p.rotation_mode = 'QUATERNION'
        p.rotation_quaternion = (1, 0, 0, 0)
        p.scale = (1, 1, 1)
    bpy.context.view_layer.update()
    base = points(meshes)
    edges = []
    offset = 0
    for obj in meshes:
        indices = np.empty(len(obj.data.edges) * 2, dtype=np.int32)
        obj.data.edges.foreach_get('vertices', indices)
        edges.append(indices.reshape((-1, 2)) + offset)
        offset += len(obj.data.vertices)
    edges = np.concatenate(edges)
    lengths = np.linalg.norm(base[edges[:, 0]] - base[edges[:, 1]], axis=1)
    valid = lengths > .0003
    edges, lengths = edges[valid], lengths[valid]
    rig = man['rig']
    report = {'slug': job['slug'], 'blend_sha256': sha(source), 'clips': [], 'transitions': [],
              'limb_roles': {k: {'head': list(arm.data.bones[v[0]].head_local),
                                 'foot': list(arm.data.bones[v[3] if len(v) > 3 else v[-1]].head_local)}
                             for k, v in rig.get('chains', {}).items()}}
    endpoints = {}
    for clip in man['clips']:
        arm.animation_data.action = bpy.data.actions[clip['name']]
        scene.render.fps = clip['fps']
        poses = []
        slipping = []
        runs = {k: [] for k in clip.get('contact_offsets', {})}
        max_scale = 0
        for frame in range(1, clip['frames'] + 1):
            scene.frame_set(frame)
            p = pose(arm)
            poses.append(p)
            max_scale = max(max_scale, max(abs(v - 1) for b in arm.pose.bones for v in b.scale))
            for key, phase in clip.get('contact_offsets', {}).items():
                chain = rig['chains'][key]
                eff = chain[3] if len(chain) > 3 else chain[-1]
                time = (frame - 1) / clip['fps']
                phase_value = ((frame - 1) / (clip['frames'] - 1) * clip.get('gait_cycles', 1) + phase) % 1
                if clip['loop'] and phase_value < clip['contact_duty']:
                    pt = p[eff][0].copy()
                    pt.x += clip['reference_speed_cm_s'] / 100 * time
                    if runs[key] and phase_value < runs[key][-1][0]:
                        slipping.append(max((v[1] - runs[key][0][1]).length for v in runs[key]) * 100)
                        runs[key] = []
                    runs[key].append((phase_value, pt))
                elif runs[key]:
                    slipping.append(max((v[1] - runs[key][0][1]).length for v in runs[key]) * 100)
                    runs[key] = []
        for seq in runs.values():
            if seq:
                slipping.append(max((v[1] - seq[0][1]).length for v in seq) * 100)
        strains, lows = [], []
        ground = job['rig_type'] not in ('aquatic', 'serpentine') or any(x in clip['suffix'] for x in ('Land', 'Settle', 'Display'))
        critical=clip['kind'] in ('run','trot','start','stop','pounce','lie','rest','up','rear','preen','swipe','scratch','collapse') or clip['suffix']=='ScratchPeck'
        sample_frames=range(1,clip['frames']+1) if critical else sorted({1+round((clip['frames']-1)*x) for x in (0,.25,.5,.75,1)})
        for frame in sample_frames:
            scene.frame_set(frame)
            verts = points(meshes)
            ratios = np.linalg.norm(verts[edges[:, 0]] - verts[edges[:, 1]], axis=1) / lengths
            strains.append(float(np.percentile(ratios, 99)))
            if ground:
                lows.append(float(verts[:, 2].min()) * 100)
        record = {'name': clip['name'], 'suffix': clip['suffix'], 'loop': clip['loop'], 'hold': clip['hold'],
                  'max_skin_p99': max(strains), 'skin_p99_samples': strains,
                  'mesh_sample_frames':list(sample_frames),'mesh_sampling':'all_frames' if critical else 'five_times',
                  'ground_min_cm': min(lows) if lows else None,
                  'max_stance_sliding_cm': max(slipping) if slipping else None,
                  'max_scale_error': max_scale,
                  'fixed_hold_drift': max((difference(poses[0], p) for p in poses), key=lambda d: d['position_cm']) if clip['hold'] and clip['kind'] in ('corpse', 'fish') else None,
                  'root_drift_cm': max((p['root'][0] - poses[0]['root'][0]).length * 100 for p in poses)}
        if clip['loop']:
            record['loop_seam'] = difference(poses[0], poses[-1])
            velocities = {n: ((poses[1][n][0] - poses[0][n][0]) * clip['fps'],
                              (poses[-1][n][0] - poses[-2][n][0]) * clip['fps']) for n in poses[0]}
            record['loop_velocity_jump_cm_s'] = max((a-b).length * 100 for a, b in velocities.values())
            record['loop_angular_speed_jump_deg_s'] = max(abs(angle(poses[0][n][1], poses[1][n][1]) - angle(poses[-2][n][1], poses[-1][n][1])) * clip['fps'] for n in poses[0])
        endpoints[clip['suffix']] = (poses[0], poses[-1])
        report['clips'].append(record)
        print('AUDIT', job['slug'], clip['suffix'], round(record['max_skin_p99'], 3), flush=True)
    for a, b in transitions(man):
        report['transitions'].append({'from': a, 'to': b, **difference(endpoints[a][1], endpoints[b][0])})
    return report


def main():
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    phase = args[0] if args else 'baseline'
    jobs = json.loads((ROOT / 'jobs.json').read_text('utf-8'))
    if len(args) > 1:
        jobs = [j for j in jobs if j['slug'] in args[1:]]
    folder = REV / phase
    folder.mkdir(parents=True, exist_ok=True)
    for job in jobs:
        result = audit(job,phase)
        (folder / f"{job['slug']}_audit.json").write_text(json.dumps(result, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
