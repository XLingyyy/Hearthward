from pathlib import Path
import json,hashlib,html
from urllib.parse import quote
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
REV=ROOT/'refinement_20261001'
def read(p,default=None):return json.loads(p.read_text('utf-8-sig')) if p.is_file() else default
def save(p,v):p.write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def link(p,label):return f'[{label}](<{p.as_posix()}>)'
def href(p):return quote(p.relative_to(ROOT).as_posix())
def esc(x):return html.escape(str(x),quote=True)
NOTES={
'stag_a':'侧看增加左右两次明确停留；卧下三段改为折肢承托姿态；倒地先失去腿部支撑再侧落，保持鹿角刚性。',
'hare':'巡视增加两次定向停留；左右倒地改为腿部先收、身体后侧落，原有跳跃与静止尸体末帧保留。',
'goat':'取消深卧时四蹄强行锁在站立位置的做法，按腹部支撑面和前伸蹄端重做低卧三段，并平滑胸腹及上肢权重、保留蹄底；探看枝叶增加视线停留，倒地分出失力与落地，同步尸体静态姿态的接地高度。',
'pheasant':'刨地改为单脚前抬、后刮、重新落足；理羽先转颈再向翼侧低喙；收敛折翼扑动，并修正起飞、短飞、落地端点。',
'pig':'重做折肢休息三段与腿部先失力的左右倒地；家猪、野猪两套速度动作和原有游戏身份保留。',
'wolf':'修正高速奔袭和起跑的尾巴穿地；长嚎增加缓起抬头、持续体态和回收；按腹部支撑面重新求解折肢卧姿及三段衔接，修整倒地节奏，长嚎仍为闭口体态。',
'black_bear':'平滑胸腹及上肢权重渐变、保留脚底权重，改善快跑、撑停与立起拉伸；短拍击从接触姿态开始并回收到支撑，完整版保留起势；修整卧姿和倒地。',
'ram':'重做前后肢折收的卧下、休息与起身；倒地先解除腿部支撑再侧落，角保持刚性，顶撞短版与完整版保留。',
'hen':'纠正ScratchPeck被通用啄食分类覆盖的错误，明确刨地、落足、啄食、回站四阶段；理羽先转颈后低喙，降低折翼逃跑扑动拉伸，修整倒地。',
'red_fox':'按关节位置纠正左右肢角色及转步内侧短步；新增两根独立耳骨和局部耳权重；修正尾尖被后脚误牵拉、按每帧压缩调整尾高；收敛扑跃并补回站，按腹部支撑面修正低卧及起身，修整倒地。',
'carp':'转向增加前躯向尾传播的曲率；短冲增加加速与衰减并接滑行；急停、上钩和提离水面分别接悬停、挣扎与离水扑动。',
'crucian_carp':'按独立体长修整传播转向、短冲衰减和停游；上钩接挣扎、提起接离水扑动，短冲结束接短暂停游。',
'catfish':'保留触须骨与原探测动作；修整传播转向、短冲和刹停，以及上钩、挣扎、提起与离水扑动的衔接。',
'eel':'增加沿全身传播的转向与弧形挣扎；补齐悬停、起游、巡游、停游的相位衔接；上钩接S形挣扎、承托提起接岸上蠕动。'
}
LIMITS={
'pheasant':{s:'已修正衔接与扑动；折翼网格仍缺少自然展翼所需翼面拓扑和展开绑定。' for s in ('Takeoff','FlightShort','Land')},
'wolf':{'Howl':'已完善抬头、持续、回收体态；可靠独立口腔/下颌张合与长嚎音频仍未提供。'},
'red_fox':{s:'已修正低卧承托与卧下/起身衔接；现有3节尾骨在卧姿仍有明显折角，自然围身蜷尾尚未完成，保留体态候选。' for s in ('CurlDown','CurlRest','GetUp')}
}
CHECK='关键奔跑、起停、扑跃、卧姿、立起、理羽、刨地、拍击和倒地逐帧检查几何，其余动作在0/25/50/75/100%抽样；全部动作逐帧检查骨骼、根与缩放，明确多段状态检查端点。'
THRESHOLDS={'ground_penetration_cm':.5,'skin_p99_edge_ratio':1.8,'cumulative_stance_sliding_cm':2,'loop_position_cm':.1,'loop_rotation_deg':.1}
def flags(c):
 f=[]
 if c['ground_min_cm'] is not None and c['ground_min_cm']<-.5:f.append('穿地')
 if c['max_skin_p99']>1.8:f.append('拉伸')
 if c['max_stance_sliding_cm'] is not None and c['max_stance_sliding_cm']>2:f.append('滑移')
 if c['root_drift_cm']>.1 or c['max_scale_error']>.0001:f.append('根/缩放')
 return f
def guide(j,m,body,qa):
 p=Path(j['guidance']);backup=REV/'backup'/p.relative_to(ROOT.parent);s=backup.read_text('utf-8')
 s=s.replace('文档版本：1.0','文档版本：1.1  \n修订日期：2026-10-01（北京时间）')
 s=s.replace('仅指导文档，动作制作与接入均未执行','动作资产已制作并完成本轮精修复核；游戏运行接入与发行验收未执行')
 s=s.replace('核对游戏主干：','设计基线主干（2026-09-30）：')
 s=s.replace('这是供后续 Codex 制作与接入动画时使用的指导文档，不是已经完成的动画或实现。当前阶段只保存 Markdown；不调用生成服务、不制作关键帧、不改骨骼/权重、不启动 UE、不导入资产、不改游戏代码与存档。','本文件同时保存动作设计要求和实际制作修订记录。本轮依据用户要求复核现有动作，修改局部曲线与必要权重，重新烘焙导出并在隔离UE项目验证；原始模型、游戏代码及存档保持原状，本轮新增云端消费为0。')
 s=s.replace('| 已解析骨骼数 |','| 原始源FBX骨骼数 |')
 s=s.replace('P0＝后续首批动作包必须覆盖的表现；','P0＝动作包的基础表现覆盖；')
 s=s.replace('P0 也不表示当前已经有对应 AnimBP、骨骼资产或动画触发器。','已交付骨骼FBX和动作序列；P0优先级不表示游戏已接入AnimBP或动画触发器。')
 s=s.replace('数量不构成当前阶段的制作任务。','实际交付数量、修订项和验证结果见第11节。')
 s=s.replace('目录为接入建议，均未创建；保持现有静态模型作为可回退表现。','SK、Skeleton和AN已在制作成果及隔离验证项目提供；游戏内目录与ABP仍为接入建议，保持静态模型作为可回退表现。')
 s=s.replace('保留源文件与原有绑定姿态、骨名、层级和权重；','保留原始源文件与可回退备份；交付骨架的角色、绑定校准与必要蒙皮修订以rig_update.json和第11节为准；')
 s=s.replace('改变骨架属于独立资产修复任务。','后续修改骨架须同步全部配对动作并重做当前文件验证。')
 s=s.replace('这些是后续验收门槛，本次只读源资产和代码；没有生成动画、导入测试、PIE或Shipping动作流畅性结果，不得把此文档标为动作验收PASS。','以上包含资产检查和后续实机验收。本轮已完成资产制作、FBX回读及隔离UE导入；PIE、Shipping、存档回归与游戏状态切换仍为NOT_RUN，实际结果见第11节。')
 s=s.replace('本次已只读核对资源文件、模型预览、FBX骨骼/动画节点以及游戏配置/移动和钓鱼逻辑；没有制作动作，也没有声称这些候选已通过流畅性或可植入性验收。执行阶段应把上述清单转成实际结果并保留失败证据。','设计基线核对来自2026-09-30；本轮已实际精修并验证当前资产指纹，提供抽样姿态及连续预览。资产检查通过不等于游戏表现接入、PIE或Shipping通过，本轮结果与受限项见第11节。')
 s=s.replace('当前11骨的连续弯曲能力必须先做小样验收；','原始源FBX为11骨；交付版已沿S形建立24节连续躯干和专用根/体/鳍骨，共28骨，已做连续波与衔接验证；')
 s+='\n## 11. 2026-10-01 精修与实际交付\n\n'+body+'\n\n'+qa+'\n'
 p.write_text(s,encoding='utf-8');j['original_guidance_sha256']=sha(backup);j['guidance_sha256']=sha(p)
 m['guidance_sha256']=j['guidance_sha256'];m['refinement_20261001']['updated_guidance_sha256']=j['guidance_sha256']
def main():
 jobs=read(ROOT/'jobs.json');prior=read(REV/'backup/制作成果/交付验收汇总.json')
 summaries=[];inventory=[];cards=[];rows=[];qa_summary=[];revised_count=0;affected_count=0;sequence_count=0
 integrity=read(REV/'source_integrity.json');assert len(integrity)==14 and all(x['pass'] for x in integrity)
 assert all(x['blend_sha256']==sha(Path(j['output'])/f"AS_{j['slug']}.blend") for j,x in zip(jobs,integrity))
 for j in jobs:
  slug=j['slug'];out=Path(j['output']);m=read(out/'animation_manifest.json');b=read(REV/'baseline'/f'{slug}_audit.json');a=read(REV/'final'/f'{slug}_audit.json');q=read(out/'fbx_roundtrip_qa.json');u=read(out/'ue_import_qa.json')
  mesh_sha=sha(out/f'SK_{slug}.fbx');blend_sha=sha(out/f'AS_{slug}.blend')
  assert a['blend_sha256']==blend_sha
  assert q['skeletal_sha256']==mesh_sha and q['structural_pass'] and len(q['clips'])==len(m['clips'])
  assert u['skeletal_sha256']==mesh_sha and u['ok'] and u['physical_scale_verified'] and all(c['ok'] for c in u['clips']) and len(u['clips'])==len(m['clips']),slug+' native QA pending'
  assert all(x['file_sha256']==c['sha256']==sha(Path(c['file'])) for x,c in zip(q['clips'],m['clips']))
  assert all(x['source_sha256']==c['sha256'] for x,c in zip(u['clips'],m['clips']))
  assert not any(x['review_flags'] for x in q['clips']) and not any(flags(x) for x in a['clips'])
  assert all(t['position_cm']<.1 and t['rotation_deg']<.1 for t in a['transitions'])
  revised=m['refinement_20261001']['revised_actions'];skin=bool(m['refinement_20261001']['skin_repair']);affected=[c['name'] for c in m['clips'] if skin or c['name'] in revised]
  revised_count+=len(revised);affected_count+=len(affected)
  loops=read(out/'loop_preview/index.json');seq=read(out/'refinement_preview/index.json',[]);sequence_count+=len(seq)
  for v in loops:
   assert next(c for c in m['clips'] if c['name']==v['action'])['loop']
   assert Path(v['path']).stat().st_mtime >= (out/f'AS_{slug}.blend').stat().st_mtime
  for v in seq:assert v['source_blend_sha256']==blend_sha and Path(v['path']).is_file()
  assert all((out/'review'/f"{c['suffix']}_{t}.png").stat().st_mtime >= (out/f'AS_{slug}.blend').stat().st_mtime for c in m['clips'] for t in (0,50,100))
  limits=LIMITS.get(slug,{});old={c['name']:c for c in b['clips']}
  compare=['| 动作 | 蒙皮拉伸P99：前→后 | 最低点cm：前→后 |','|---|---:|---:|']
  fmt=lambda v:'—' if v is None else f'{v:.3f}'
  for c in a['clips']:
   o=old[c['name']]
   if flags(o) or c['suffix'] in ('Rest','CurlRest','RearSniff_Hold'):
    compare.append(f"| {c['suffix']} | {o['max_skin_p99']:.3f} → {c['max_skin_p99']:.3f} | {fmt(o['ground_min_cm'])} → {fmt(c['ground_min_cm'])} |")
  gate='\n'.join('- '+s+'：'+r for s,r in limits.items()) or '本物种没有新增模型能力门槛；G动作按原设计在对应游戏事件接入后启用。'
  body=f"保留 **{len(m['clips'])} 段动作**，本轮修订 **{len(revised)} 段曲线**，交付骨架 **{m['rig']['bone_count']} 骨**。{NOTES[slug]}\n\n原目录：{link(out,'动物资产目录')}；{link(out/f'AS_{slug}.blend','可编辑Blender源')}；{link(out/f'SK_{slug}.fbx','当前配套SK')}；{link(out/'animation_manifest.json','动作清单与接触/阶段标记')}。\n\n上一轮从指导展开动作库，核实骨骼与蒙皮、建立固定根、接触IK和物种曲线，烘焙导出并校准站姿。本轮由完整备份重新读取实际动作，补查五个时点及关键动作逐帧变形，修复曲线、端点和必要权重后重新导出。角色与权重详见{link(out/'rig_update.json','骨架更新记录')}。\n\n"+'\n'.join(compare)+f"\n\n标记仅供视觉对齐，不自动触发伤害、声音、钓鱼或物品结算。原始顶点、拓扑、UV经逐网格哈希核对保持一致，约束已烘焙移除。回退版本：{link(REV/'backup'/out.relative_to(ROOT.parent),'原版本备份')}。\n\n能力门槛：\n\n{gate}"
  qa=f"当前文件验证：**FBX {len(m['clips'])}/{len(m['clips'])} 结构通过，UE {len(m['clips'])}/{len(m['clips'])} 原生导入与压缩姿态采样通过，未解决数值标记为0。**{CHECK}\n\n证据：{link(out/'fbx_roundtrip_qa.json','FBX回读')}、{link(out/'ue_import_qa.json','UE原生结果')}、{link(REV/'final'/f'{slug}_audit.json','逐帧及端点复核')}、{link(out/'source_integrity_20261001.json','几何与UV核对')}、{link(ROOT/'动物动作预览.html','更新预览入口')}。\n\n游戏AnimInstance、权威事件适配、PIE、Shipping、实际相机/地形、幼体、暂停/读档与奖励单次结算回归：**NOT_RUN**。"
  guide(j,m,body,qa)
  table=['| 动作 | 本轮处理 | 秒 / FPS | 状态 |','|---|---|---:|---|'];frames=[]
  for c in m['clips']:
   reason=limits.get(c['suffix']);state='受限体态候选' if reason else 'G接入候选' if c['priority']=='G' else '资产检查通过'
   kind='曲线修订' if c['name'] in revised else '蒙皮复核' if skin else '复核保留'
   c['delivery_review']={'state':state,'reasons':[reason] if reason else [],'native_source_verified':True,'fbx_source_verified':True,'numeric_qa':'PASS','visual_check':'start/middle/end samples reviewed; critical continuous previews; no gameplay certification'}
   if 'refinement_20261001' in c:c['refinement_20261001']['qa']='PASS_CURRENT_ASSET_CHECKS'
   table.append(f"| {link(Path(c['file']),c['name'])} | {kind} | {c['duration']:.3f} / {c['fps']} | {state}{'：'+reason if reason else ''} |")
   imgs=''.join(f'<img loading="lazy" src="{href(out/"review"/f"{c["suffix"]}_{t}.png")}" alt="{esc(c["suffix"])} {t}%">' for t in (0,50,100))
   changes='；'.join(c.get('refinement_20261001',{}).get('changes',[]))
   frames.append(f'<details class="clip" data-search="{esc(c["name"]+c["description"])}"><summary>{esc(c["suffix"])} · {esc(c["description"])} <small>{state} / {kind}</small></summary><div class="frames">{imgs}</div><p>{esc(reason or changes)}</p><a href="{href(Path(c["file"]))}">动作FBX</a></details>')
  m['ue_import']='PASS';m['visual_qa']='POSE_SAMPLES_REVIEWED; continuous critical previews; capability gates retained';m['refinement_20261001']['checks']='PASS_CURRENT_ASSET_CHECKS';save(out/'animation_manifest.json',m)
  rev=read(out/'refinement_20261001.json');rev.update(checks='PASS_CURRENT_ASSET_CHECKS',affected_actions=affected,thresholds=THRESHOLDS,original_guidance_sha256=j['original_guidance_sha256'],updated_guidance_sha256=j['guidance_sha256']);save(out/'refinement_20261001.json',rev)
  extra=f"\n\n## 导入与使用\n\n1. 当前SK与本物种专用Skeleton配对。{'狐狸新增两根耳骨，须同步使用当前SK和全部21个动作，不混用旧37骨Skeleton。' if slug=='red_fox' else '蒙皮更新后须复核配对动作。'}\n2. SK以Skeletal Mesh导入；分动作FBX用Animation Only并指定同一Skeleton。微小BindPoseProxy三角形只保存绑定矩阵，不作为游戏网格。\n3. FBX内部为厘米，Scale=1；按清单使用30/60FPS，关闭默认30FPS重采样。Blender编辑场景为米，+X前、+Z上。\n4. Root Motion关闭，世界位置由Actor或后续鱼线/承托层提供。参考速度与接触相位见清单，播放率按实际位移速度校准；当前没有伤害结算Notify。\n5. G动作及受限体态按能力门槛启用。游戏组件、AnimInstance、碰撞、动态引用和cook须另行接入测试。\n\n原生验证项目：E:/AiAgent/XLingGame/.animal-qa/HW/qa/pipeline/UEQA/UEQA.uproject；物种路径 /Game/AnimalMotionRefined20261001/{slug}。原始模型和存档未改动，本轮新增云端消费为0。\n"
  (out/'交付与接入说明.md').write_text(f"# {j['name']}动作交付与接入说明｜2026-10-01\n\n"+body+'\n\n'+qa+'\n\n## 当前动作清单\n\n'+'\n'.join(table)+extra,encoding='utf-8')
  videos=''.join(f'<figure><video controls muted loop preload="metadata"><source src="{href(Path(v["path"]))}" type="video/mp4"></video><figcaption>{esc(v["action"])} · 10次连续循环 · {v["native_fps"]}FPS</figcaption></figure>' for v in loops)
  refined=''.join(f'<figure><video controls muted preload="metadata"><source src="{href(Path(v["path"]))}" type="video/mp4"></video><figcaption>{esc(v["name"])} · {v["duration_s"]:.2f}秒<br>{esc(" → ".join(x["action"].removeprefix("AN_"+slug+"_") for x in v["segments"]))}</figcaption></figure>' for v in seq)
  cards.append(f'<section id="{slug}"><h2>{esc(j["name"])} <small>{len(m["clips"])}动作 · {m["rig"]["bone_count"]}骨 · 修订{len(revised)}段曲线</small></h2><p>{esc(NOTES[slug])}</p><p><a href="../{quote(Path(j["guidance"]).name)}">更新指导</a> · <a href="{href(out/"交付与接入说明.md")}">交付说明</a></p><div class="videos">{videos}</div>{"<h3>精修连续展示</h3><div class=videos>"+refined+"</div>" if refined else ""}<p class="gate">{esc("；".join(limits.values()))}</p><details><summary>全部动作的开始/中间/结束姿态</summary>{"".join(frames)}</details></section>')
  nbad=sum(bool(flags(c)) for c in b['clips'])
  summaries.append({'name':j['name'],'slug':slug,'clips':len(m['clips']),'bones':m['rig']['bone_count'],'curve_refined':len(revised),'affected_actions':len(affected),'baseline_numeric_flags':nbad,'current_numeric_flags':0,'fbx_roundtrip_current':True,'native_import_current':True,'source_unchanged':True,'guidance_updated':True,'capability_gates':limits,'folder':str(out),'videos':loops,'refinement_previews':seq})
  rows.append(f"| {j['name']} | {len(m['clips'])} | {len(revised)} | {m['rig']['bone_count']} | {nbad} → 0 | {link(out/'交付与接入说明.md','资产与记录')} |")
  files=[out/f'SK_{slug}.fbx',out/f'AS_{slug}.blend',out/'animation_manifest.json',out/'rig_update.json',out/'fbx_roundtrip_qa.json',out/'ue_import_qa.json',out/'refinement_20261001.json',out/'source_integrity_20261001.json',out/'交付与接入说明.md',Path(j['guidance'])]+[Path(c['file']) for c in m['clips']]+[Path(v['path']) for v in loops+seq]
  inventory.append({'slug':slug,'files':[{'path':str(p),'bytes':p.stat().st_size,'sha256':sha(p)} for p in files],'original_source_sha256':j['source_sha256'],'original_guidance_sha256':j['original_guidance_sha256'],'updated_guidance_sha256':j['guidance_sha256']})
  qa_summary.append({'slug':slug,'structural_pass':True,'clips':len(m['clips']),'flags':{}})
 save(ROOT/'jobs.json',jobs);save(ROOT/'roundtrip_summary.json',qa_summary);save(ROOT/'资产完整性清单.json',inventory)
 result={'schema':'hearthward.animal.motion.delivery.v2','revision':'2026-10-01','animals':14,'clips':303,'curve_refined_actions':revised_count,'affected_actions_including_skin':affected_count,'native_current_verified_clips':303,'source_unchanged':True,'all_fbx_current':True,'all_native_current':True,'fresh_pose_reviews':303,'numeric_review_flags_clips':0,'baseline_expanded_review_flags':sum(x['baseline_numeric_flags'] for x in summaries),'cloud_downloaded_references':prior['cloud_downloaded_references'],'cloud_confirmed_credits':prior['cloud_confirmed_credits'],'refinement_additional_cloud_credits':0,'gait_loop_videos':28,'refinement_sequence_videos':sequence_count,'gameplay_integration':'NOT_RUN','animals_summary':summaries}
 save(ROOT/'交付验收汇总.json',result);save(REV/'revision_summary.json',result)
 report=f"# 14种动物动作精修复核｜2026-10-01\n\n已回顾14份指导与303段实际动作，修订 **{revised_count}段骨骼曲线**；计入局部蒙皮变更，复核受影响动作 **{affected_count}段**。保留原目录及303个动作名称，更新14份指导、可编辑Blender源、配套FBX、清单与交付说明。\n\n最终 **303/303 FBX回读结构通过，303/303 UE 5.8.2原生导入和压缩姿态采样通过**。扩展检查发现的{result['baseline_expanded_review_flags']}个数值问题片段已消除，当前未解决数值标记为0。明确多段端点检查通过。\n\n入口：{link(ROOT/'动物动作预览.html','动物动作预览')}；{link(ROOT/'交付验收汇总.json','验证汇总')}；{link(ROOT/'资产完整性清单.json','当前资产指纹')}。更新28段慢/快循环预览，新增{sequence_count}段精修连续展示，303组三帧姿态和关键动作五时点、双侧视图。\n\n| 动物 | 总动作 | 曲线修订 | 骨骼 | 数值问题：前→后 | 入口 |\n|---|---:|---:|---:|---:|---|\n"+'\n'.join(rows)+f"\n\n## 制作过程与修正\n\n上一轮从指导展开303个动作，核实骨骼角色、修正固定根和部分绑定，采用接触IK与物种曲线制作、烘焙和站姿绑定校准，再做FBX和隔离UE检查。开始/中间/结束三帧抽样遗漏了部分高速压缩姿态，指导文件也仍写着未制作。\n\n本轮先备份474个文件，读取真实烘焙动作；发现尾巴穿地、卧姿与胸腹拉伸、母鸡刨地顺序被分类覆盖、雉鸡起飞/短飞跳变。修正折肢卧姿、尾尖/后脚误绑、胸腹渐变、鸟类刨地/理羽、短拍击回收和倒地失力节奏；补齐鱼类传播转向、起停波、上钩/提起衔接及狐狸耳部控制。\n\n{CHECK}支撑滑移采用整个支撑区间累计值。阈值：穿地≤0.5cm、蒙皮边长拉伸P99≤1.8、累计足端滑移≤2cm、循环首尾位置≤0.1cm与角度≤0.1°，根保持单位缩放。循环有限差分速度记录只作诊断，同时提供10次循环视频，未将其宣称为完整运动学认证。\n\n原始源FBX、顶点、拓扑、UV校验一致，约束已烘焙移除。本轮新增云端消费 **0**；上一轮确认消费{prior['cloud_confirmed_credits']}点保留为历史记录。\n\n## 能力门槛与接入状态\n\n- 雉鸡Takeoff、FlightShort、Land：衔接已修正；折翼源网格仍缺少完整展翼拓扑与绑定，保留G候选。\n- 狼Howl：抬头、持续、回收体态已精修；可靠下颌/口腔张合和音频仍未提供。\n- 游戏骨骼组件、AnimInstance、实际地形/玩家相机、幼体、暂停/读档、奖励单次结算、PIE和Shipping：NOT_RUN。本轮完成动作资产修订及隔离原生验证。\n\n## 备份与复现\n\n{link(REV/'backup_index.json','474文件备份索引')}；{link(REV/'baseline','修订前扩展测量')}；{link(REV/'final','最终逐帧与端点记录')}；{link(REV/'scripts','制作和检查脚本')}；{link(REV/'source_integrity.json','几何及UV验证')}。\n\n各动物的refinement_20261001.json记录前后FBX指纹与备份路径。回退应整体恢复动物AS、SK、clips、骨架/动作清单及指导，狐狸新增耳骨后尤其不能混用旧37骨Skeleton。\n"
 report=report.replace('- 游戏骨骼组件','- 狐狸CurlDown、CurlRest、GetUp：低卧承托和衔接已修正；3节尾骨仍有折角，自然围身蜷尾尚未完成，保留体态候选。\n- 游戏骨骼组件')
 report+=f"\n连续视频均以Blender视频序列器回读，核对实际FPS和时长，并从交付MP4抽取8个时点复查：{link(REV/'video_encoding_qa.json','46段视频编码检查')}；{link(REV/'连续视频检查','连续视频姿态图')}；{link(REV/'visual_review_20261001.json','视觉复核记录')}。\n\n复现依赖的原制作辅助脚本已按原内容快照保存，版本及哈希见{link(REV/'script_provenance.json','脚本来源记录')}。probe开头的脚本和fix_wolf_rest.py保留为探索历史；最终源资产使用refine_motion.py从本轮完整备份生成，历史尾部补丁遇到最终承托姿态会直接退出。\n"
 report+=f"\n旧版鲤鱼、鲫鱼、鲶鱼的Burst被误作10次循环。3段旧视频已移到{link(REV/'historical_previews/index.json','历史预览索引')}，字节校验保持一致；当前快游循环改用SwimCruise，Burst仍是一次性加速/衰减动作。正式预览以各动物当前index.json为准。\n\n最终文件、14份指导、全部303段FBX、原生验证指纹、46段视频及预览链接的一致性检查见{link(REV/'final_delivery_verification.json','交付一致性检查')}。\n"
 (ROOT/'精修复核报告_20261001.md').write_text(report,encoding='utf-8');(ROOT/'制作交付说明.md').write_text(report,encoding='utf-8')
 css='body{margin:0;background:#111821;color:#e0e9f4;font:16px system-ui,"Microsoft YaHei",sans-serif}main{max-width:1250px;margin:auto;padding:30px}a{color:#7dd9d0}h2{font-size:25px}small{font-size:14px;color:#a6b7ca;font-weight:normal}section{background:#1b2532;border:1px solid #34445a;border-radius:12px;padding:24px;margin:24px 0}.videos{display:grid;grid-template-columns:repeat(auto-fit,minmax(290px,1fr));gap:15px}figure{margin:0;background:#10171f;border-radius:8px;overflow:hidden}video{width:100%;display:block}figcaption{padding:10px;font-size:13px;color:#b7cada}.frames{display:flex;gap:6px}.frames img{width:calc(33.333% - 4px)}summary{cursor:pointer;padding:12px 0}.clip{border-top:1px solid #37475b}.gate{color:#ffc886}input{box-sizing:border-box;width:100%;padding:14px;background:#1b2532;border:1px solid #557087;border-radius:8px;color:white;font-size:17px}nav{display:flex;flex-wrap:wrap;gap:14px;margin:20px 0}p{line-height:1.7}'
 nav=''.join(f'<a href="#{j["slug"]}">{esc(j["name"])}</a>' for j in jobs)
 page=f'<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>动物动作精修复核2026-10-01</title><style>{css}</style><main><h1>14种动物 · 动作精修复核</h1><p>303段已复核 · 修订{revised_count}段曲线 · FBX与UE原生检查通过 · 28段10次循环 + {sequence_count}段精修连续展示</p><p>预览来自当前Blender资产；世界位移、鱼线/承托和游戏事件仍需接入。完整展翼与长嚎口腔/音频门槛见对应卡片。</p><p><a href="精修复核报告_20261001.md">复核报告</a> · <a href="交付验收汇总.json">验证汇总</a></p><nav>{nav}</nav><input id="search" placeholder="搜索动物、动作名或说明">{"".join(cards)}</main><script>document.getElementById("search").oninput=e=>{{const q=e.target.value.toLowerCase();document.querySelectorAll("section").forEach(s=>{{const animal=s.querySelector("h2").textContent.toLowerCase().includes(q);let any=false;s.querySelectorAll(".clip").forEach(c=>{{const ok=animal||c.dataset.search.toLowerCase().includes(q);c.hidden=!ok;if(ok)any=true;if(q&&ok)c.open=true;}});s.hidden=!!q&&!animal&&!any;}});}};</script></html>'
 (ROOT/'动物动作预览.html').write_text(page,encoding='utf-8')
 print(json.dumps({k:v for k,v in result.items() if k!='animals_summary'},ensure_ascii=False,indent=2))
if __name__=='__main__':main()

