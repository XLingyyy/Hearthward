"""Publish R2 documentation only after exact asset and preview QA passes."""
from pathlib import Path
import json,hashlib,html,math
from urllib.parse import quote
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2';R1=ROOT/'refinement_20261001'
CHANGED={'stag_a','hare','goat','pig','wolf','black_bear','ram','red_fox'}
NOTES={
 'stag_a':'默认头部从侧偏改为朝 +X 前方；同步修正绑定与全部22段动作。奔跑、起跑和停止采用左右分离的四拍落足。',
 'hare':'快速跳跃、起跳和停止分开左右脚时序；慢跳也同步增加左右先后差，避免前脚或后脚成对锁在一起。',
 'goat':'奔跑和起停分开左右脚。重做卧下、深卧休息、起身：关节保持连接，四肢沿各自身体侧折收，使用连续旋转基准，消除跨中线和过渡翻转。',
 'pig':'家猪与野猪两套快速移动均分开左右前后脚，起跑与停止同步匹配新版跑步。',
 'wolf':'奔袭、起跑和停止改为四足独立的支撑与摆动轨迹；保留既有尾部净空和抬头体态修订。',
 'black_bear':'快跑、起跑和撑停分开左右落足，并保持掌底接触和已有蒙皮修订。',
 'ram':'默认头部改为朝 +X 前方，修正局部绑定与全部25段动作；奔跑和起停分开左右落足；同步修正侧躺时角部接地高度。',
 'red_fox':'默认头部改为朝 +X 前方，修正局部绑定与全部21段动作；保留独立耳骨和尾尖权重。快速移动分开左右落足，侧躺同步重算净空。'
}
def read(p):return json.loads(p.read_text('utf-8-sig'))
def save(p,v):p.write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def link(p,label):return f'[{label}](<{p.as_posix()}>)'
def href(p):return quote(p.relative_to(ROOT).as_posix())
def esc(v):return html.escape(str(v),quote=True)
def audit_flags(c):
 return (c['max_skin_p99']>1.8 or (c['ground_min_cm'] is not None and c['ground_min_cm']<-.5) or (c['max_stance_sliding_cm'] or 0)>2 or c['root_drift_cm']>.1 or c['max_scale_error']>.0001 or c.get('loop_seam',{}).get('position_cm',0)>.1 or c.get('loop_seam',{}).get('rotation_deg',0)>.1)
def markers(c):
 duty=c.get('contact_duty');offsets=c.get('contact_offsets',{})
 if duty is None or not offsets:return
 result=[];cycles=c.get('gait_cycles',1)
 for foot,offset in offsets.items():
  for base in range(-1,cycles+1):
   for name,p in [('Contact',0),('Lift',duty)]:
    t=(base+p-offset)/cycles
    if 0<=t<1:result.append({'name':foot+'_'+name,'time_s':round(t*c['duration'],6),'kind':'visual_contact'})
 c['events']['authored_markers']=sorted(result,key=lambda e:e['time_s'])
 c['events']['marker_authority']='offline visual gait phase reference; not gameplay triggers'
def video(v,caption):
 buttons=''.join(f'<button type="button" data-rate="{r}">{label}</button>' for r,label in [(1,'正常速度'),(.5,'0.5×'),(.25,'0.25×')])
 return f'<figure><video controls muted playsinline preload="metadata" src="{href(Path(v["path"]))}"></video><figcaption>{esc(caption)}</figcaption><div class="rates">{buttons}</div></figure>'
def main():
 jobs=read(ROOT/'jobs.json');prior=read(REV/'backup/制作成果/交付验收汇总.json');target=read(REV/'targeted_qa.json');source_records={x['slug']:x for x in read(REV/'source_integrity.json')}
 after=[x for x in target['gaits'] if x['phase']=='after'];assert all(x['pass'] for x in after) and len(after)==10
 sole=read(REV/'skin_sole_phase_qa.json');assert len(sole)==9 and all(x['pass'] for x in sole)
 assert all(c['max_wrong_side_intrusion_cm']==0 and c['joint_connection_error_cm']<.001 for c in target['goat_rest']['after']['clips'])
 assert all(x['position_cm']<.1 and x['rotation_deg']<.1 for p in target['start_stop_seams'].values() for x in p)
 videoqa=read(REV/'video_encoding_qa.json');assert len(videoqa)==64 and all(v['pass'] and sha(Path(v['path']))==v['sha256'] for v in videoqa)
 summaries=[];inventory=[];cards=[];rows=[];rt=[];native=[];changed_total=0
 for j in jobs:
  slug=j['slug'];out=Path(j['output']);m=read(out/'animation_manifest.json');revised=slug in CHANGED
  fbx=read(out/'fbx_roundtrip_qa.json');ue=read(out/'ue_import_qa.json')
  assert fbx['structural_pass'] and not any(c['review_flags'] for c in fbx['clips']),slug
  assert fbx['skeletal_sha256']==ue['skeletal_sha256']==sha(out/f'SK_{slug}.fbx') and ue['ok'] and ue['physical_scale_verified'],slug
  assert len(fbx['clips'])==len(ue['clips'])==len(m['clips']),slug
  for c,a,b in zip(m['clips'],fbx['clips'],ue['clips']):
   assert c['name']==a['name']==b['name'] and c['sha256']==a['file_sha256']==b['source_sha256']==sha(Path(c['file'])) and b['ok'],c['name']
  audit_path=REV/'final'/f'{slug}_audit.json'
  if not revised:
   a=read(R1/'final'/f'{slug}_audit.json');assert a['blend_sha256']==sha(out/f'AS_{slug}.blend');a['reused_unchanged_from']=str(R1/'final'/f'{slug}_audit.json');save(audit_path,a)
  audit=read(audit_path);assert audit['blend_sha256']==sha(out/f'AS_{slug}.blend') and not any(audit_flags(c) for c in audit['clips'])
  assert all(t['position_cm']<.1 and t['rotation_deg']<.1 for t in audit['transitions'])
  loops=read(out/'loop_preview/index.json');seq=read(out/'refinement_preview/index.json') if (out/'refinement_preview/index.json').is_file() else []
  correction=read(out/'correction_preview/index.json') if revised else [];digest=sha(out/f'AS_{slug}.blend')
  if revised:
   assert source_records[slug]['pass'] and source_records[slug]['blend_sha256']==digest
   thumbnails=read(out/'review/source_record_20261001_r2.json');assert thumbnails['source_blend_sha256']==digest and len(thumbnails['images'])==len(m['clips'])*3
   assert all(v['source_blend_sha256']==digest for v in loops+seq+correction),(slug,'stale preview')
  else:
   for v in loops:v['source_blend_sha256']=digest;v['source_equivalence']='native blend and paired FBX hashes unchanged since R1'
   save(out/'loop_preview/index.json',loops)
  previous=read(REV/'backup'/out.relative_to(ROOT.parent)/'animation_manifest.json') if revised else m
  changed=m.get('correction_20261001_r2',{}).get('changed_actions',[]) if revised else [];changed_total+=len(changed)
  capability=next(x for x in prior['animals_summary'] if x['slug']==slug)['capability_gates']
  if revised:
   guide=Path(j['guidance']);base=REV/'backup'/guide.relative_to(ROOT.parent);text=base.read_text('utf-8')
   text=text.replace('文档版本：1.1','文档版本：1.2')
   text=text.replace('修订日期：2026-10-01（北京时间）','修订日期：2026-10-01（北京时间；R2问题修正）')
   entry='\n\n当前修订为R2。第11节保留R1历史记录，第12节记录本次用户指出问题的实际修正；导入与预览以当前配套资产及清单为准。\n'
   first=text.find('\n');text=text[:first]+entry+text[first:]
   body=f'\n\n## 12. 2026-10-01 R2：头部、快速步态与深卧问题修正\n\n{NOTES[slug]}\n\n本轮修订{len(changed)}段动作，完整复核本动物{len(m["clips"])}段。可编辑源、配套模型、动作FBX、清单和预览均已更新。\n'
   if m['correction_20261001_r2']['head_bind']:
    h=m['correction_20261001_r2']['head_bind']
    body+=f'\n头颈局部默认偏航校正 {h["yaw_deg"]:+.1f}°，更新 {h["changed_head_vertices"]} 个受头部骨骼影响的顶点及对应绑定矩阵。模型顶点位置在该局部发生了实际变化；拓扑、UV、权重、身体和四肢比例保留。模型测量长度从 {h["old_length_m"]*100:.2f}cm 变为 {h["new_length_m"]*100:.2f}cm，属于朝向改变后的外包围范围变化，未整体缩放动物。\n\n导入时必须同步使用当前SK、Skeleton与本物种全部动作；禁止将旧绑定模型与新动作混用。\n'
   gaitrows=['| 动作 | 左右相位间隔 | 支撑占比 | 当前参考速度 cm/s |','|---|---:|---:|---:|']
   for c in m['clips']:
    if 'gait_revision' in c:
     gaitrows.append(f'| {c["name"]} | 25% 周期 | {c["contact_duty"]:.2f} | {c["reference_speed_cm_s"]:.3f} |')
   body+='\n'+'\n'.join(gaitrows)+'\n\n支撑与摆动分别求解四只脚；快步左右前脚、左右后脚各错开四分之一周期。慢跑原有左右半周期交替保留。参考速度与支撑时长已重算，Actor位移或播放率应按当前清单校准。起跑→奔跑→停止→待机的端点已检查匹配；记录的接触时间仅供离线视觉参考，不自动触发游戏结算。\n'
   if slug=='goat':
    b=target['goat_rest']['before']['clips'][1];a=target['goat_rest']['after']['clips'][1]
    body+=f'\n山羊三段卧姿以同一折肢末姿态衔接，折腿时允许足端收拢，不再用无明确弯曲方向的IK强行压到腹部最低平面。膝肘越过身体中线的最大距离 {b["max_wrong_side_intrusion_cm"]:.3f}cm → {a["max_wrong_side_intrusion_cm"]:.3f}cm；全部三段逐帧未发现跨中线关节。连续旋转基准消除了接近反向时的翻转；卧下/起身的最大相邻帧旋转约8°。深卧仍保留腹下空间，避免强压躯干造成穿模。\n'
   body+=f'\n验证：{len(m["clips"])}/{len(m["clips"])} FBX回读和UE 5.8.2原生导入/压缩姿态采样通过；固定根、单位缩放、循环和多段端点检查通过。原始Tripo源FBX校验不变，本轮新增云端消费0。游戏运行接入仍为NOT_RUN。\n\n当前入口：'+link(out/'交付与接入说明.md','配套资产说明')+'；'+link(ROOT/'问题修正复核报告_20261001_R2.md','本次复核报告')+'；'+link(ROOT/'动物动作预览.html','连续预览')+'。\n'
   guide.write_text(text.rstrip()+body,encoding='utf-8');j['r2_baseline_guidance_sha256']=previous['guidance_sha256'];j['guidance_sha256']=sha(guide);m['guidance_sha256']=j['guidance_sha256']
   for c in m['clips']:
    if c['name'] in changed:
     c['correction_20261001_r2']['qa']='PASS_CURRENT_ASSET_CHECKS';c['delivery_review'].update(numeric_qa='PASS',native_source_verified=True,fbx_source_verified=True,visual_check='R2 native pose samples and decoded continuous videos reviewed')
     checked=next(a for a in fbx['clips'] if a['name']==c['name'])
     c['qa']={'status':'PASS_CURRENT_ASSET_CHECKS','source_fbx_sha256':c['sha256'],'max_ground_penetration_m':max(0,-checked['ground_min_z_m']) if checked['ground_min_z_m'] is not None else None,'root_translation_drift_m':checked['root_translation_drift_m'],'root_rotation_drift_deg':checked['root_rotation_drift_deg'],'max_adjacent_rotation_deg':checked['max_adjacent_world_rotation_deg'],'skin_edge_stretch_p99':checked['skin_edge_stretch_p99'],'max_stance_sliding_m':checked['max_stance_sliding_m'],'fbx_roundtrip_report':str(out/'fbx_roundtrip_qa.json'),'native_import_report':str(out/'ue_import_qa.json')}
     if c['loop']:c['qa'].update(loop_pose_error_deg=checked['loop_pose_error_deg'],loop_position_error_m=checked['loop_position_error_m'])
     markers(c)
   m['current_revision']='2026-10-01 R2'
   m['refinement_20261001'].update(checks='HISTORICAL_R1_SUPERSEDED_BY_R2',historical_record=True)
   m['ue_import']='PASS';m['visual_qa']='R2_THREE_REPORTED_ISSUES_REVIEWED; CURRENT_CONTINUOUS_VIDEOS'
   rev=m['correction_20261001_r2'];rev.update(checks='PASS_CURRENT_ASSET_CHECKS',blend_sha256=digest,skeletal_sha256=sha(out/f'SK_{slug}.fbx'),guidance_sha256=j['guidance_sha256'],targeted_qa=str(REV/'targeted_qa.json'),native_qa=str(out/'ue_import_qa.json'))
   save(out/'correction_20261001_r2.json',rev)
   r1=read(out/'refinement_20261001.json');r1.update(historical_record=True,checks='HISTORICAL_R1_SUPERSEDED_BY_R2',current_revision_record=str(out/'correction_20261001_r2.json'));save(out/'refinement_20261001.json',r1)
   table=['| 当前动作FBX | 本次处理 | 秒 / FPS | 状态 |','|---|---|---:|---|']
   for c in m['clips']:
    state='受限体态候选：'+capability[c['suffix']] if c['suffix'] in capability else c['delivery_review']['state']
    table.append(f'| {link(Path(c["file"]),c["name"])} | {"本次修正" if c["name"] in changed else "复核保留"} | {c["duration"]:.3f} / {c["fps"]} | {state} |')
   note=f'# {j["name"]}动作交付与接入说明｜2026-10-01 R2\n\n{NOTES[slug]}\n\n指导文档：'+link(Path(j['guidance']),'v1.2指导及R2记录')+f'。本次修订{len(changed)}段，配套共{len(m["clips"])}段。\n\n当前模型：'+link(out/f'SK_{slug}.fbx','绑定模型')+'；编辑源：'+link(out/f'AS_{slug}.blend','Blender动作源')+'；'+link(out/'animation_manifest.json','动作参数与SHA清单')+'。\n\n'+'\n'.join(table)+f'\n\n导入时使用本文件列出的当前SK、同一Skeleton和动作。厘米单位、Scale=1，按清单保留30/60FPS，关闭默认30FPS重采样。根固定，Actor负责世界位移。快速动作的参考速度与支撑时长已更新；颜色足端标记仅用于预览，正式模型没有新增颜色或标记。完整展翼、长嚎口腔/音频、狐狸自然围尾能力门槛仍保留。\n\n隔离原生验证目录 /Game/AnimalMotionCorrected20261001R2/{slug}；当前{len(m["clips"])}/{len(m["clips"])}段通过。游戏组件接入、实际地形和发行构建未运行。本轮新增云端消费0。\n\n'+link(out/'correction_20261001_r2.json','本次修订数据')+'；'+link(out/'source_integrity_20261001_r2.json','几何、UV和权重验证')+'；'+link(REV/'backup_index.json','修正前备份索引')+'。\n'
   (out/'交付与接入说明.md').write_text(note,encoding='utf-8');save(out/'animation_manifest.json',m)
  videos_html=''.join(video(v,v['name']+' · '+('正面' if v['view']=='front' else '侧面')) for v in correction)
  loops_html=''.join(video(v,v['action']+' · 10次循环') for v in loops)
  seq_html=''.join(video(v,v['name']+' · 连续动作') for v in seq)
  frames=[]
  for c in m['clips']:
   imgs=''.join(f'<img loading="lazy" src="{href(out/"review"/f"{c["suffix"]}_{pct}.png")}" alt="{esc(c["suffix"])} {pct}%">' for pct in [0,50,100])
   frames.append(f'<details class="clip" data-search="{esc(c["name"]+c["description"])}"><summary>{esc(c["suffix"])} · {esc(c["description"])} <small>{c["fps"]}FPS · {c["duration"]:.3f}s</small></summary><div class="frames">{imgs}</div><p>{esc("；".join(c.get("delivery_review",{}).get("reasons",[])))}</p><a href="{href(Path(c["file"]))}">动作FBX</a></details>')
  cards.append(f'<section id="{slug}"><h2>{esc(j["name"])} <small>{len(m["clips"])}动作 · {len(changed)}段R2修正</small></h2><p>{esc(NOTES.get(slug,"本次模型和动作保留，原资产验证指纹保持一致。"))}</p><p><a href="../{quote(Path(j["guidance"]).name)}">当前指导</a> · <a href="{href(out/"交付与接入说明.md")}">配套资产说明</a></p>{("<p>足端标记：蓝＝左前，红＝右前，绿＝左后，橙＝右后。可使用0.25×检查左右先后顺序。</p><div class=videos>"+videos_html+"</div>") if correction else ""}<details><summary>慢速 / 快速连续循环</summary><div class="videos">{loops_html}</div></details>{("<details><summary>其他连续动作</summary><div class=videos>"+seq_html+"</div></details>") if seq else ""}<p class="gate">{esc("；".join(set(capability.values())))}</p><details><summary>全部动作的开始 / 中间 / 结束姿态</summary>{"".join(frames)}</details></section>')
  summaries.append({'name':j['name'],'slug':slug,'clips':len(m['clips']),'bones':rig_count if (rig_count:=m['rig']['bone_count']) else 0,'r2_changed_actions':len(changed),'current_numeric_flags':0,'fbx_roundtrip_current':True,'native_import_current':True,'original_source_unchanged':True,'head_geometry_changed':bool(m.get('correction_20261001_r2',{}).get('head_bind')) if revised else False,'guidance_version':'1.2' if revised else '1.1','capability_gates':capability,'folder':str(out),'videos':loops,'refinement_previews':seq,'correction_previews':correction})
  rows.append(f'| {j["name"]} | {len(m["clips"])} | {len(changed)} | {link(out/"交付与接入说明.md","资产和修订记录")} |')
  rt.append({'slug':slug,'structural_pass':True,'clips':len(m['clips']),'flags':{}})
  native.append({'slug':slug,'ok':True,'clips':len(m['clips']),'skeletal_sha256':ue['skeletal_sha256'],'native_revalidated_r2':revised})
  files=[out/f'SK_{slug}.fbx',out/f'AS_{slug}.blend',out/'animation_manifest.json',out/'rig_update.json',out/'rig_audit.json',out/'fbx_roundtrip_qa.json',out/'ue_import_qa.json',out/'source_integrity_20261001.json',out/'refinement_20261001.json',out/'交付与接入说明.md',Path(j['guidance'])]
  files += [Path(c['file']) for c in m['clips']]+[out/'review'/f'{c["suffix"]}_{pct}.png' for c in m['clips'] for pct in [0,50,100]]
  files += [Path(v['path']) for v in loops+seq+correction]+[out/category/'index.json' for category in ['loop_preview','refinement_preview','correction_preview'] if (out/category/'index.json').is_file()]
  if revised:files += [out/'correction_20261001_r2.json',out/'source_integrity_20261001_r2.json',out/'review/source_record_20261001_r2.json']
  inventory.append({'slug':slug,'files':[{'path':str(p),'bytes':p.stat().st_size,'sha256':sha(p)} for p in files],'original_source_sha256':j['source_sha256'],'updated_guidance_sha256':j['guidance_sha256']})
 assert changed_total==88 and sum(s['clips'] for s in summaries)==303
 save(ROOT/'jobs.json',jobs);save(ROOT/'roundtrip_summary.json',rt);save(ROOT/'ue_summary.json',native);save(ROOT/'资产完整性清单.json',inventory)
 result={'schema':'hearthward.animal.motion.delivery.v3','revision':'2026-10-01 R2','animals':14,'clips':303,'r2_changed_animals':8,'r2_changed_actions':88,'r2_corrected_head_models':3,'native_current_verified_clips':303,'native_revalidated_r2_clips':189,'native_reused_unchanged_clips':114,'original_tripo_sources_unchanged':True,'local_head_vertices_changed':True,'topology_uv_weights_preserved':True,'all_fbx_current':True,'all_native_current':True,'fresh_pose_reviews_r2':189,'current_pose_reviews':303,'numeric_review_flags_clips':0,'gait_loop_videos':28,'refinement_sequence_videos':18,'correction_sequence_videos':18,'cloud_confirmed_credits':prior['cloud_confirmed_credits'],'cloud_downloaded_references':prior['cloud_downloaded_references'],'r2_additional_cloud_credits':0,'gameplay_integration':'NOT_RUN','animals_summary':summaries}
 save(ROOT/'交付验收汇总.json',result);save(REV/'revision_summary.json',result)
 report=f'# 动物模型与动作问题修正｜2026-10-01 R2\n\n已修正用户指出的三类问题，更新8种四足动物的可编辑源和相关成果，共88段动作。保留14种动物、303段动作及原目录名称。\n\n- 雄鹿A、公羊、赤狐：校正默认头颈朝向及局部模型绑定，全部68段配对动作同步重定向。默认模型与待机现在朝前；有意侧看仍保留，并回到新的朝前基准。\n- 八种四足动物：快速移动、起跑和停止使用四条独立足端轨迹，左右前脚和左右后脚各错开25%周期；实测约23%～25%，替代旧版约4%～12%的接近同步状态。兔子的慢跳也同步处理。既有慢跑左右半周期交替保留。\n- 山羊：重做卧下、深卧休息和起身的折肢末姿态和连续旋转路径。逐帧膝肘跨中线最大偏移19.626cm→0，关节连接误差小于0.001cm；消除过渡翻转，卧下/起身相邻帧最大旋转约8°。同步减少肢体卷入躯干和关节错位表现。\n\n入口：'+link(ROOT/'动物动作预览.html','当前连续预览')+'；'+link(REV/'头部修正前后.png','三种头部前后对照')+'；'+link(REV/'山羊卧姿连续检查.png','山羊连续姿态')+'。\n\n| 动物 | 当前总动作 | 本次修正动作 | 入口 |\n|---|---:|---:|---|\n'+'\n'.join(rows)+'\n\n## 当前资产验证\n\n本次重新检查受影响的189段完整动作库，FBX回读和UE 5.8.2原生导入/压缩姿态采样均通过。另114段未改变动作保留上一轮验证，模型与动作SHA逐项一致；因此当前303段配套资产验证有效。全部当前数值复核标记为0，未将导入成功作为艺术质量或游戏运行接入的证明。\n\n关键动作逐帧检查骨骼与变形网格；其余网格在五个时点抽样。固定根、单位缩放、接地、支撑累计滑移、循环首尾，以及起跑→奔跑→停止→待机、卧下→休息→起身端点检查通过。阈值沿用穿地≤0.5cm、蒙皮边长比P99≤1.8、支撑累计滑移≤2cm、接缝≤0.1cm/0.1°；旋转相邻帧超过60°会标记复查。山羊另逐帧检查关节连接与身体侧别，并抽样检查下段腿部与躯干非相邻三角形交叉。该分区检查不代表所有网格全局自交证明，连续正/侧面视频已人工复查。\n\n新增18段本次问题的正面/侧面连续展示，更新受影响的16段慢/快循环和9段既有连续展示；加上保留的其他预览，共64段视频。全部MP4回读核对真实FPS、时长，并从实际文件抽帧检查。足端颜色只用于预览，正式模型没有附加标记。\n\n## 导入与参数\n\n三种头部修正同时更新了SK、绑定矩阵和所有配对动作，必须成套导入同一物种的新Skeleton。不能把旧SK/旧绑定与本次动作混用。三种模型只有头部影响区域的顶点位置改变；拓扑、UV和权重保持一致，原始Tripo源FBX保持原状。其他五种动物模型几何保持原状。\n\n快速动作的支撑占比改变后参考速度已重算。当前动画清单中的reference_speed_cm_s、contact_duty、contact_offsets与离线接触标记已同步更新，后续Actor世界位移或播放率必须使用当前参数。标记不自动触发伤害、奖励或其他游戏结算。\n\n## 记录、备份与范围\n\n八份相关指导已更新为v1.2，第12节写入本次实际修正、参数和验证；其他六份指导保留v1.1。R1精修报告和各动物R1记录已标为历史记录，当前成果以R2、当前文件与完整性清单为准。\n\n'+link(REV/'backup_index.json','313文件修正前备份')+'；'+link(REV/'scripts','复现及验证脚本')+'；'+link(REV/'final','原生曲线测量')+'；'+link(REV/'targeted_qa.json','本次问题专项测量')+'；'+link(REV/'source_integrity.json','局部模型变化与源保护')+'；'+link(REV/'video_encoding_qa.json','64段视频编码及抽帧检查')+'；'+link(ROOT/'资产完整性清单.json','当前资产指纹')+'。\n\n仍保留上一轮能力门槛：雉鸡完整展翼拓扑与绑定、狼长嚎的口腔/音频、狐狸自然围身蜷尾。游戏组件接入、PIE、实际地形及发行构建未运行。本次新增云端消费为0。\n'
 report+='\n另直接检查9段快速动作的掌底/蹄底网格顶点轨迹，确认左右脚的分离存在于实际蒙皮输出中，而非仅存在于预览标记或清单相位：'+link(REV/'skin_sole_phase_qa.json','实际足底轨迹检查')+'。最终文件、模型/动作配对指纹、指导、全部视频及备份一致性见 '+link(REV/'final_delivery_verification.json','最终交付一致性检查')+'。\n'
 (ROOT/'问题修正复核报告_20261001_R2.md').write_text(report,encoding='utf-8');(ROOT/'制作交付说明.md').write_text(report,encoding='utf-8')
 old=(REV/'backup/制作成果/精修复核报告_20261001.md').read_text('utf-8');(ROOT/'精修复核报告_20261001.md').write_text('> 本文为R1历史记录。当前模型、动作和验证结果已由R2更新，见 '+link(ROOT/'问题修正复核报告_20261001_R2.md','当前R2报告')+'。\n\n'+old,encoding='utf-8')
 css='body{margin:0;background:#111923;color:#dce8f4;font:16px system-ui,"Microsoft YaHei",sans-serif}main{max-width:1240px;margin:auto;padding:28px}a{color:#7de0cf}p{line-height:1.75}small{color:#9eb5c9;font-size:14px}section{margin:24px 0;padding:24px;background:#1c2837;border:1px solid #355069;border-radius:12px}.videos{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:16px}figure{margin:0;background:#101720;border-radius:8px;overflow:hidden}video{width:100%;display:block}figcaption,.rates{padding:9px;font-size:14px}button{padding:6px 12px;border:1px solid #58778f;background:#21374a;color:white;border-radius:5px;margin-right:8px;cursor:pointer}summary{cursor:pointer;padding:14px 0}.clip{border-top:1px solid #38526b}.frames{display:flex;gap:6px}.frames img{width:calc(33.333% - 4px)}nav{display:flex;flex-wrap:wrap;gap:14px;margin:22px 0}input{box-sizing:border-box;width:100%;padding:14px;background:#21374a;border:1px solid #567992;color:white;border-radius:8px;font:inherit}.comparison{width:100%;border-radius:9px}.gate{color:#ffcc89}'
 nav=''.join(f'<a href="#{j["slug"]}">{esc(j["name"])}</a>' for j in jobs)
 page=f'<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>动物头部、步态与卧姿修正 R2</title><style>{css}</style><main><h1>头部、步态与卧姿修正 · R2</h1><p>三种头部朝前 · 八种四足动物左右落足分离 · 山羊深卧及过渡修正</p><p>88段动作修订，303段当前资产验证有效；本次重新验证189段，另114段文件未变。64段连续视频可查看正常或慢放。</p><p><a href="问题修正复核报告_20261001_R2.md">本次修正报告</a> · <a href="交付验收汇总.json">验证汇总</a></p><details open><summary>头部修正前后对照</summary><img class="comparison" src="{href(REV/"头部修正前后.png")}" alt="雄鹿A、公羊和赤狐头部修正前后"></details><nav>{nav}</nav><input id="search" placeholder="搜索动物、动作名或说明">{"".join(cards)}</main><script>document.querySelectorAll("[data-rate]").forEach(b=>b.onclick=()=>b.closest("figure").querySelector("video").playbackRate=Number(b.dataset.rate));document.getElementById("search").oninput=e=>{{const q=e.target.value.toLowerCase();document.querySelectorAll("section").forEach(s=>{{const a=s.querySelector("h2").textContent.toLowerCase().includes(q);let any=false;s.querySelectorAll(".clip").forEach(c=>{{const ok=a||c.dataset.search.toLowerCase().includes(q);c.hidden=!ok;any=any||ok;if(q&&ok)c.open=true;}});s.hidden=!!q&&!a&&!any;}});}};</script></html>'
 (ROOT/'动物动作预览.html').write_text(page,encoding='utf-8')
 print(json.dumps({k:v for k,v in result.items() if k!='animals_summary'},ensure_ascii=False,indent=2))
if __name__=='__main__':main()

