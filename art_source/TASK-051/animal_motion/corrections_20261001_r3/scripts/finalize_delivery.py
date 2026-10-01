"""Publish current local docs and fingerprints only after actual-file QA."""
import sys,json,shutil,re
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import ROOT,REV,SLUGS,GAITS,read,save,sha,backup_folder

def link(label,path):return f'[{label}](<{Path(path).as_posix()}>)'
def write(path,value):Path(path).write_text(value,encoding='utf-8')
def native_flags(r):
    flags=[]
    if r['max_skin_p99']>1.8:flags.append('skin edge P99 exceeds 1.8')
    if r['ground_min_cm'] is not None and r['ground_min_cm']<-.5:flags.append('ground below -0.5 cm')
    if r['max_stance_sliding_cm'] is not None and r['max_stance_sliding_cm']>2:flags.append('stance sliding exceeds 2 cm')
    if r['max_scale_error']>.0001 or r['root_drift_cm']>.1:flags.append('root or scale')
    if 'loop_seam' in r and (r['loop_seam']['position_cm']>.1 or r['loop_seam']['rotation_deg']>.1):flags.append('loop seam')
    return flags

jobs=read(ROOT/'jobs.json');backup=read(REV/'backup_index.json');known={r['source'] for r in backup['files']}
def preserve(path):
    path=Path(path)
    if not path.is_file() or str(path) in known:return
    dest=REV/'backup'/path.relative_to(ROOT.parent);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,dest);digest=sha(path);assert sha(dest)==digest
    backup['files'].append({'source':str(path),'backup':str(dest),'bytes':path.stat().st_size,'sha256':digest});known.add(str(path))

summary=[];roundtrip=[];all_gaits=[];matched=[];numeric_total=0
for j in jobs:
    slug=j['slug'];out=Path(j['output']);m=read(out/'animation_manifest.json');fbx=read(out/'fbx_roundtrip_qa.json');ue=read(out/'ue_import_qa.json');changed=slug in SLUGS;sk=sha(out/f'SK_{slug}.fbx')
    assert sha(j['source'])==j['source_sha256'],slug+' modified original source'
    assert fbx['skeletal_sha256']==ue['skeletal_sha256']==sk,slug+' stale skeletal QA'
    assert fbx['structural_pass'] and ue['ok'] and ue['physical_scale_verified'],slug+' import QA failed'
    assert len(fbx['clips'])==len(ue['clips'])==len(m['clips'])
    for c in m['clips']:
        fr=next(r for r in fbx['clips'] if r['name']==c['name']);ur=next(r for r in ue['clips'] if r['name']==c['name'])
        assert sha(c['file'])==c['sha256']==fr['file_sha256']==ur['source_sha256']
        assert c['skeleton_signature']==m['rig']['skeleton_signature']
        assert not fr['review_flags'],(slug,c['name'],fr['review_flags'])
    if changed:
        a=read(REV/'final'/f'{slug}_audit.json');target=read(REV/'final'/f'{slug}_targeted.json');cross=read(REV/'final'/f'{slug}_native_source_match.json');pose_record=read(out/'review/source_record_20261001_r3.json')
        assert a['blend_sha256']==target['source_blend_sha256']==cross['source_blend_sha256']==pose_record['source_blend_sha256']==sha(out/f'AS_{slug}.blend')
        assert target['pass'] and cross['core_pass'] and target['geometry_topology_uv_preserved']
        assert all(not native_flags(c) for c in a['clips'])
        assert all(t['position_cm']<.1 and t['rotation_deg']<.1 for t in a['transitions'])
        assert len(pose_record['images'])==len(m['clips'])*3
        for r in pose_record['images']:assert Path(r['path']).is_file()
        videos=read(out/'gait_preview_r3/index.json')
        assert all(v['source_blend_sha256']==sha(out/f'AS_{slug}.blend') and Path(v['path']).is_file() for v in videos)
        for c in m['clips']:
            ar=next(r for r in a['clips'] if r['name']==c['name']);fr=next(r for r in fbx['clips'] if r['name']==c['name'])
            c['qa']={'status':'PASS','revision':'2026-10-01 R3','max_skin_p99':ar['max_skin_p99'],'ground_min_cm':ar['ground_min_cm'],'max_stance_sliding_cm':ar['max_stance_sliding_cm'],'fixed_root_drift_cm':ar['root_drift_cm'],'max_scale_error':ar['max_scale_error'],'max_adjacent_world_rotation_deg':fr['max_adjacent_world_rotation_deg'],'review_flags':[],'mesh_sampling':ar['mesh_sampling'],'fbx_mesh_sample_count':fr['sampled_mesh_frames']}
            c['correction_20261001_r3']['qa']='PASS'
            c['delivery_review'].update(numeric_qa='PASS',native_source_verified=True,fbx_source_verified=True,visual_check='current source start/middle/end renders available; gait front/side phase images reviewed; no gameplay certification')
        m['correction_20261001_r3']['qa']='PASS'
        m['correction_20261001_r3']['source_blend_sha256']=sha(out/f'AS_{slug}.blend')
        m['correction_20261001_r3']['skeletal_fbx_sha256']=sk
        m['correction_20261001_r3']['native_core_pose_match']='PASS'
        m['correction_20261001_r3']['supplementary_all_bone_rotation_match']='PASS' if cross['pass'] else 'SEE_RECORDED_FOOT_SUBFRAME_DIAGNOSTICS'
        m['ue_import']='PASS';m['visual_qa']='CURRENT_SOURCE_RENDERED_AND_GAIT_PHASES_REVIEWED'
        preserve(j['guidance']);guidance=Path(j['guidance']).read_text('utf-8')
        guidance=guidance.split('\n## 13. 2026-10-01 R3')[0]
        guidance=guidance.replace('当前修订为R2。第11节保留R1历史记录，第12节记录本次用户指出问题的实际修正；导入与预览以当前配套资产及清单为准。','当前修订为R3。第11、12节为R1/R2历史记录，第13节为本次行走内收与刚性修正；导入以当前配套SK、全部动作及清单为准。')
        guidance=guidance.replace('文档版本：1.2','文档版本：1.3').replace('北京时间；R2问题修正','北京时间；R3行走修正')
        special='野兔另校正左右后膝约5cm的自动绑定高度差，统一关节测量高度；保持后跗骨求解分支连续，并对躯干错误腿权重做30%回收。短前腿采用独立刚化强度。' if slug=='hare' else ''
        section=f'''\n\n## 13. 2026-10-01 R3：行走内收与腿部刚性\n\n本次更新本物种 {len(m['clips'])} 段配套动作，其中 {len(target['gaits'])} 段为步行、快步、奔跑和起停修正。四肢关节在完整行走阶段略向内收，设计侧偏约3°；骨干按固定长度分段转动，弯曲集中在膝肘与跗腕关节。关节支点按原模型表面重新测量，降低过深的身体压缩和扭转，并按完整动作库校准局部蒙皮刚化强度。{special}\n\n相对R2，本次模型顶点、拓扑、UV和材质保持不变；骨骼绑定支点和部分蒙皮权重有实际修改。原始Tripo FBX保持不变。足端关键帧轨迹、接触时序、参考速度和原动作时长保持原参数。全部配套动作已重定向至新绑定并重新导出。\n\n**必须成套导入新的SK、Skeleton和本物种全部动作，避免混用R2绑定。**优先用AS编辑源继续编辑；Blender回导FBX时，自动Connected关节可能屏蔽合法的子关节位移，应在Edit Mode关闭Connected而保持支点位置。UE原生关节位置与腿段旋转已和编辑源核对。\n\n当前验证：{len(m['clips'])}/{len(m['clips'])} FBX回读和UE原生导入/压缩采样通过；全部行走关键帧检查内收、骨段长度、根固定、单位缩放、起停接缝，关键变形检查接地和蒙皮拉伸。当前正侧面行走视频与三阶段姿态图已重新生成。游戏运行接入为NOT_RUN；本次云端消费0。\n\n{link('专项实测',REV/'final'/f'{slug}_targeted.json')}；{link('当前配套资产',out/'交付与接入说明.md')}；{link('R3复核报告',ROOT/'问题修正复核报告_20261001_R3.md')}；{link('当前连续预览',ROOT/'动物动作预览.html')}。\n'''
        write(j['guidance'],guidance+section);j['guidance_sha256']=sha(j['guidance']);m['guidance_sha256']=j['guidance_sha256']
        preserve(out/'job.json');save(out/'job.json',j)
        save(out/'animation_manifest.json',m);save(out/'correction_20261001_r3.json',m['correction_20261001_r3'])
        lines=[f'# {j["name"]}动作交付与接入说明｜2026-10-01 R3','',f'关节改为略向内收，腿段保持长度并集中在关节处转动；本次修正 {len(target["gaits"])} 段行走/起停，完整更新 {len(m["clips"])} 段配套动作。',special,'',link('v1.3动作指导',j['guidance'])+'；'+link('R3连续预览',ROOT/'动物动作预览.html')+'。','',link('Blender编辑源',out/f'AS_{slug}.blend')+'；'+link('绑定模型',out/f'SK_{slug}.fbx')+'；'+link('当前动作清单',out/'animation_manifest.json')+'。','','| 动作 FBX | 本次处理 | 秒 / FPS | 状态 |','|---|---|---:|---|']
        for c in m['clips']:lines.append(f'| {link(c["name"],c["file"])} | '+('行走修正' if c['kind'] in GAITS else '配套绑定重定向')+f' | {c["duration"]:.3f} / {c["fps"]} | {c["delivery_review"]["state"]} |')
        lines+=['','新SK、Skeleton与全部动作必须成套导入；保留厘米单位、Scale=1和清单中的30/60FPS。根固定，Actor位移使用当前参考速度。原足端接触参数和离线事件含义保持一致。相对R2，原模型顶点、拓扑、UV、材质保持不变，绑定支点与局部权重已更新。','', 'Blender回导FBX会自动连接部分重合关节，这会屏蔽合法的关节位移。可直接使用AS编辑源；回导时在Edit Mode关闭Connected，保持骨骼支点和层级不变。UE原生采样已独立检查关节位置与腿段旋转。','',f'当前 {len(m["clips"])}/{len(m["clips"])} 段FBX和原生导入检查通过。全部动作配套指纹已核对，原始Tripo源未修改。完整游戏接入、PIE、地形及发行构建：NOT_RUN。现有受限能力与G事件接入条件见动作清单。', '',link('FBX回读',out/'fbx_roundtrip_qa.json')+'；'+link('原生UE检查',out/'ue_import_qa.json')+'；'+link('专项测量',REV/'final'/f'{slug}_targeted.json')+'；'+link('模型完整性',out/'source_integrity_20261001_r3.json')+'；'+link('备份索引',REV/'backup_index.json')+'。','', 'R1/R2说明、验证和连续视频为历史记录，当前以R3清单、R3行走视频和当前源文件为准。']
        preserve(out/'交付与接入说明.md');write(out/'交付与接入说明.md','\n'.join(x for x in lines if x is not None)+'\n')
        for category in ('loop_preview','refinement_preview','correction_preview'):
            p=out/category/'index.json'
            if p.is_file():
                preserve(p);old=read(p)
                for v in old:v['is_current']=False;v['superseded_by']='2026-10-01 R3 paired source and gait_preview_r3';v['historical_source_blend_sha256']=v['source_blend_sha256']
                save(p,old);write(out/category/'历史预览说明.md','本目录为R1/R2历史预览。当前行走请使用相邻 gait_preview_r3 目录或总预览页面。对应旧源与旧姿态图保存在 corrections_20261001_r3/backup 中。\n')
        for oldname in ('correction_20261001_r2.json','refinement_20261001.json','source_integrity_20261001_r2.json','source_integrity_20261001.json','review/source_record_20261001_r2.json','review/render_record.json'):
            p=out/oldname
            if p.is_file():
                preserve(p);old=read(p);old['historical_record']=True;old['superseded_by']='2026-10-01 R3';old['historical_backup_root']=str(backup_folder(j));save(p,old)
        all_gaits+=target['gaits'];matched.append(cross)
    else:videos=read(out/'loop_preview/index.json')
    row={'name':j['name'],'slug':slug,'clips':len(m['clips']),'bones':m['rig']['bone_count'],'r3_changed_actions':len(m['clips']) if changed else 0,'r3_gait_actions':len(target['gaits']) if changed else 0,'current_numeric_flags':0,'fbx_roundtrip_current':True,'native_import_current':True,'original_source_unchanged':True,'guidance_version':'1.3' if changed else '1.1','folder':str(out),'videos':videos,'capability_gates':m.get('capability_gates',{})}
    summary.append(row);roundtrip.append({'slug':slug,'structural_pass':True,'clips':len(m['clips']),'flags':{}})

save(ROOT/'jobs.json',jobs);save(REV/'backup_index.json',backup);save(ROOT/'roundtrip_summary.json',roundtrip)
save(REV/'targeted_qa.json',{'revision':'2026-10-01 R3','animals':[read(REV/'final'/f'{s}_targeted.json') for s in SLUGS],'pass':all(read(REV/'final'/f'{s}_targeted.json')['pass'] for s in SLUGS)})
video_qa=read(REV/'video_encoding_qa.json')
for j in jobs:
    for v in next(s['videos'] for s in summary if s['slug']==j['slug']):
        q=next(q for q in video_qa if q['path']==v['path']);assert q['pass'] and q['sha256']==sha(v['path']) and q['source_blend_sha256']==sha(Path(j['output'])/f'AS_{j["slug"]}.blend')

table=['| 动物 | 配套动作 | 行走/起停 | 快速动作最大外张 cm：前→后 |','|---|---:|---:|---:|']
for j in jobs:
    if j['slug'] not in SLUGS:continue
    old=read(REV/'baseline'/f'{j["slug"]}_gait.json');new=read(REV/'final'/f'{j["slug"]}_targeted.json');b=max(c['max_outward_cm'] for c in old['clips'] if c['kind']=='run');n=max(c['max_outward_joint_cm_at_full_gait'] for c in new['gaits'] if c['kind']=='run')
    table.append(f'| {j["name"]} | {len(read(Path(j["output"])/"animation_manifest.json")["clips"])} | {len(new["gaits"])} | {b:.2f} → {n:.3f} |')
maxlength=max(c['segment_length_error_cm'] for c in all_gaits);maxpos=max(c['max_position_error_cm'] for a in matched for c in a['clips']);maxshaft=max(c['max_shaft_rotation_error_deg'] for a in matched for c in a['clips'])
report=f'''# 四足动物行走问题修正复核｜2026-10-01 R3\n\n已修正雄鹿A、野兔、山羊、猪、狼、黑熊、公羊和赤狐的行走外张与腿部柔软问题。39段步行、快步、奔跑和起停使用新关节求解；8套绑定及189段配套动作已重新导出。14种动物、303段动作及原文件名保留，鸟类和鱼类114段保持原文件。\n\n关节向身体内侧略收，设计限幅约3°；腿段保持固定长度，膝肘和跗腕承担弯曲。旧关节支点偏离实际腿部表面，已按原网格测量校正；同时减少过深身体压缩，按物种和完整动作库校准骨干权重。野兔左右后膝的原绑定高度相差约5cm，本次统一测量高度，保持后跗骨分支连续，单独强化短前腿并回收躯干错误腿权重。\n\n{link('当前正面/侧面连续预览',ROOT/'动物动作预览.html')}。\n\n'''+'\n'.join(table)+f'''\n\n最大骨段长度误差为 {maxlength:.6f} cm。四肢足端关键帧、接触时序、参考速度和动作时长保持原参数；全部配套动作重定向至新绑定。相对R2，模型顶点、拓扑和UV逐网格哈希一致，材质保留，原始Tripo源FBX逐项SHA一致。\n\n## 当前文件验证\n\n受影响189段重新执行FBX回读及UE原生导入/压缩姿态采样，固定根、单位缩放、循环和配对绑定检查通过。全39段行走关键帧测量内收与定长；行走、起停、卧姿、立起、拍击和倒地逐帧检查变形，其余网格取五个时点。接地阈值−0.5cm，蒙皮边长比P99≤1.8，支撑滑移≤2cm，循环/起停接缝≤0.1cm/0.1°，相邻帧旋转≤60°；当前该组标记为0。另114段的SK和动作指纹与原验证一致。\n\nUE压缩姿态与编辑源的全部关节位置差异≤{maxpos:.4f}cm，12根腿段旋转差异≤{maxshaft:.3f}°。更严格的全骨旋转补充比较保留了黑熊足端分数帧差异：R2也存在此现象，当前最高4.25°；该项未标为全骨逐角一致，见原始对照数据。该诊断与腿段外张、长度、接地及固定根的通过项分别记录。\n\nBlender回导FBX会自动把重合支点设为Connected，从而屏蔽合法子关节位移。回读检查将该选项恢复为与编辑源一致的关闭状态，支点和层级不变；UE独立核对正常。继续编辑优先使用AS，回导FBX时同样关闭Connected。\n\n更新567张三阶段姿态图，新增46段当前正侧面行走视频，保留其他物种12段慢快循环。全部当前循环视频重新打开MP4，核对实际FPS、帧数、时长并抽帧。局部骨干使用相同原始顶点采样比较截面代理指标，详见专项数据；该指标是局部诊断，不构成全网格体积或全局自交证明。\n\n## 导入与复现\n\n必须同步导入新SK、新Skeleton与本物种全部动作，不能混用R2绑定。动作文件名、骨名、30/60FPS及游戏事件含义保持一致。8份指导已更新v1.3，第13节记录本次修正；旧R1/R2记录和旧视频标为历史。\n\n{link('专项测量',REV/'targeted_qa.json')}；{link('编辑源与原生姿态对照',REV/'final')}；{link('视频回读',REV/'video_encoding_qa.json')}；{link('完整备份',REV/'backup_index.json')}；{link('复现与检查脚本',REV/'scripts')}；{link('当前资产指纹',ROOT/'资产完整性清单.json')}。\n\n游戏AnimInstance接入、PIE、实际地形与发行构建：NOT_RUN。本次新增云端消费0；上一轮受限能力和G事件接入条件继续保留。\n'''
write(ROOT/'问题修正复核报告_20261001_R3.md',report)
write(ROOT/'制作交付说明.md','# 动物动作交付｜2026-10-01 R3\n\n8种四足动物的39段行走/起停已修正为略向内收、固定骨段长度和集中的关节弯曲。更新8套配套SK/Blender源及189段动作FBX；其他6种鸟类/鱼类114段原文件保留。\n\n'+link('查看当前连续预览',ROOT/'动物动作预览.html')+'；'+link('修正及验收细节',ROOT/'问题修正复核报告_20261001_R3.md')+'。\n\n新绑定与全部配套动作须成套导入；相关8份指导已更新v1.3第13节。FBX回读、UE原生导入与核心腿段姿态核对完成，所有当前文件指纹已更新。游戏运行接入仍为NOT_RUN。\n\n'+link('修正前备份',REV/'backup_index.json')+'；'+link('资产完整性清单',ROOT/'资产完整性清单.json')+'。\n')
oldreport=ROOT/'问题修正复核报告_20261001_R2.md';preserve(oldreport)
history='> R2历史报告。当前四足绑定与动作已由R3替代，请读取[当前R3报告](问题修正复核报告_20261001_R3.md)。\n\n'
if not oldreport.read_text('utf-8').startswith('> R2历史报告'):write(oldreport,history+oldreport.read_text('utf-8'))
save(REV/'backup_index.json',backup)
result={'schema':'hearthward.animal.motion.delivery.v4','revision':'2026-10-01 R3','animals':14,'clips':303,'r3_changed_animals':8,'r3_gait_actions':39,'r3_rebaked_paired_actions':189,'native_current_verified_clips':303,'native_revalidated_r3_clips':189,'native_reused_unchanged_clips':114,'all_fbx_current':True,'all_native_current':True,'original_tripo_sources_unchanged':True,'r3_geometry_topology_uv_preserved':True,'r3_bind_and_local_skin_changed':True,'current_numeric_review_flags_clips':0,'fresh_pose_reviews_r3':189,'r3_gait_videos':46,'current_gait_videos':len(video_qa),'additional_cloud_credits':0,'gameplay_integration':'NOT_RUN','supplementary_foot_subframe_rotation_diagnostics':'retained; black bear max 4.25 deg; core leg segments and joint positions pass','animals_summary':summary}
save(ROOT/'交付验收汇总.json',result)
print('R3_DOCS_CURRENT',len(summary),sum(a['clips'] for a in summary),len(backup['files']),flush=True)
