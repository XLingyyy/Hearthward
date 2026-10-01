"""Publish auditable per-animal handoffs and a local gallery from real results."""
from pathlib import Path
import json,hashlib,html,sys
from urllib.parse import quote
from collections import Counter

ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
NOTES={
 'stag_a':'保留鹿角刚性，采用四拍慢行、奔逃及分段颈部动作。独立嘴唇/下颌变形未提供。',
 'hare':'按前后肢错开的跳跃相位制作，耳部低幅跟随，避免把野兔缩放成四足走路模板。',
 'goat':'修正尾部骨骼误位及蹄底权重；缩小吃草时的颈部弯折，保留趴卧与起身过渡。吃草可用于低矮灌木表现，尚未校准到具体植物接触点。',
 'pheasant':'核实原 Limb 链为鸟腿，增加左右各三节翼骨。当前原网格为折翼形状，真实展开飞行仍需重新制作翼面拓扑。',
 'pig':'一份模型分别提供家猪与野猪移动、低头觅食、鼻部前探及倒地动作；不包含独立开口与口腔结构。',
 'wolf':'对角小跑、奔袭、嗅探和闭口前探咬击分开制作；咬击采用指导文档允许的闭口替代。长嚎只含抬头动作，尚无可靠张口与音频。',
 'black_bear':'重步、左右挥爪、嗅探、立起候选与倒地分别处理；修正胸腹受到腿骨误牵拉的权重。',
 'ram':'修正前腿链和蹄部错绑，保留角部刚性，低头警告与短版/完整顶撞分别制作。',
 'hen':'增加左右各三节折翼骨，提供双足走跑、啄食、刨地、理羽及沙浴；展开短飞仍受原折翼网格限制。',
 'red_fox':'修正前后肢编号误识，新增三节尾骨，移除耳名骨对全身的错误残余权重；独立耳部转动暂未提供，警觉由头颈姿态表达。',
 'carp':'重新绑定十二节连续躯干骨与左右鳍骨，按躯干到尾部递增的波幅制作游动。',
 'crucian_carp':'按三十厘米鱼体单独重新绑定，游动幅度与节奏独立于鲤鱼；实测 UE 导入尺寸。',
 'catfish':'重建连续躯干与左右鳍骨，并保留重绑的两条三节触须链，提供触须探测动作。',
 'eel':'沿原始 S 形鱼体建立二十四节连续骨链，保留曲线静止形状，使用沿体传播的游动波。',
}
LIMITS={
 'pheasant':{'FlightShortG':'原网格为折翼形状，候选片段不具备自然展翼飞行所需翼面拓扑。'},
 'hen':{'FlightShortG':'原网格为折翼形状，候选片段不具备自然展翼飞行所需翼面拓扑。'},
 'wolf':{'Howl':'仅抬头体态；尚无可靠口腔张合和已授权声音。'},
}
FLAG_CN={'contact slip exceeds 2 cm/frame':'接触滑移超过 2 厘米/帧','visible ground penetration':'抽样姿态出现地面穿入','p99 skin edge strain exceeds 1.8':'蒙皮边长拉伸的第 99 百分位超过 1.8','large adjacent world-space joint rotation':'相邻帧关节旋转过大'}

def read(p,default=None):
    return json.loads(p.read_text(encoding='utf-8')) if p.is_file() else default
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,v):p.write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
def link(p,label):return f'[{label}](<{p.as_posix()}>)'
def href(p):return quote(p.relative_to(ROOT).as_posix())
def escaped(s):return html.escape(str(s),quote=True)

def main():
    strict='--draft' not in sys.argv
    jobs=read(ROOT/'jobs.json');summary=[];inventory=[];cards=[];rows=[];credits=0;cloud_success=0
    assert len(jobs)==14
    total=0;native_total=0;flag_count=0;visual_count=0;source_ok=True;native_ok=True;structural_ok=True
    for j in jobs:
        out=Path(j['output']);m=read(out/'animation_manifest.json');qa=read(out/'fbx_roundtrip_qa.json',{});ue=read(out/'ue_import_qa.json',{})
        source_same=sha(Path(j['source']))==j['source_sha256'];guide_same=sha(Path(j['guidance']))==j['guidance_sha256'];source_ok &= source_same and guide_same
        fbx={c['name']:c for c in qa.get('clips',[])};native={c['name']:c for c in ue.get('clips',[])}
        mesh_sha=sha(out/f"SK_{j['slug']}.fbx")
        native_current=bool(ue.get('ok') and ue.get('physical_scale_verified') and ue.get('skeletal_sha256')==mesh_sha and len(native)==len(m['clips']) and all(native.get(c['name'],{}).get('source_sha256')==c['sha256'] for c in m['clips']))
        fbx_current=bool(qa.get('structural_pass') and len(fbx)==len(m['clips']) and all(fbx.get(c['name'],{}).get('file_sha256')==c['sha256'] for c in m['clips']))
        native_ok &= native_current;structural_ok &= fbx_current
        count=len(m['clips']);total+=count;native_total+=sum(c.get('ok',False) for c in native.values()) if native_current else 0
        cloud=read(out/'cloud_source.json',{});credits+=cloud.get('animation_credits',0) or 0;credits+=cloud.get('rig_credits',0) or 0;cloud_success+=int(cloud.get('state')=='downloaded')
        gates=[];clip_lines=['| 动作 | 用途 | 秒 / FPS | 优先级 | 检查 / 接入状态 |','|---|---|---|---|---|']
        visual_frames=[];clip_sha=[]
        for c in m['clips']:
            assert Path(c['file']).is_file() and sha(Path(c['file']))==c['sha256']
            record=fbx.get(c['name'],{});flags=[FLAG_CN.get(x,x) for x in record.get('review_flags',[])]
            special=LIMITS.get(j['slug'],{}).get(c['suffix'])
            if special:flags.append(special)
            if flags:gates.append({'name':c['name'],'reasons':flags})
            flag_count+=int(bool(record.get('review_flags')))
            state='需细修' if flags else '玩法候选' if c['priority']=='G' else '工程检查通过' if native_current and fbx_current else '检查进行中'
            c['delivery_review']={'state':state,'reasons':flags,'native_source_verified':native_current,'fbx_source_verified':fbx_current,'visual_check':'start/middle/end rendered poses; gait cycle video for locomotion, not all-clips continuous artist certification'}
            label=link(Path(c['file']),c['name'])
            clip_lines.append(f"| {label} | {c['description']} | {c['duration']:.3f} / {c['fps']} | {c['priority']} | {state}{'：'+'；'.join(flags) if flags else ''} |")
            clip_sha.append({'name':c['name'],'path':c['file'],'sha256':c['sha256'],'state':state})
            frame_paths=[out/'review'/f"{c['suffix']}_{v}.png" for v in (0,50,100)]
            assert all(p.is_file() for p in frame_paths)
            fresh=all(p.stat().st_mtime >= (out/f"AS_{j['slug']}.blend").stat().st_mtime for p in frame_paths)
            visual_count+=int(fresh)
            if strict:assert fresh, 'Stale action review: '+c['name']
            visual_frames.append(f'<details class="action" data-search="{escaped(c["name"]+c["description"])}"><summary>{escaped(c["suffix"])} · {escaped(c["description"])} <span class="badge">{state}</span></summary><div class="frames">'+''.join(f'<img loading="lazy" src="{href(p)}" alt="{escaped(c["description"])} {n}">' for p,n in zip(frame_paths,['开始','中间','结束']))+'</div>'+('<p class="flag">'+escaped('；'.join(flags))+'</p>' if flags else '')+f'<p>{c["duration"]:.3f} 秒 · {c["fps"]} FPS · <a href="{href(Path(c["file"]))}">动作 FBX</a></p></details>')
        m['visual_qa']='POSE_SAMPLES_REVIEWED; flagged/candidate clips are listed in delivery_review'
        save(out/'animation_manifest.json',m)
        videos=read(out/'loop_preview'/'index.json',[])
        assert len(videos)==2
        for v in videos:
            p=Path(v['path']);assert p.is_file() and p.stat().st_size>10000
            if strict:assert p.stat().st_mtime >= (out/f"AS_{j['slug']}.blend").stat().st_mtime, 'Stale video: '+str(p)
        actual_size=ue.get('mesh_inspection',{}).get('payload',{}).get('inspection',{}).get('skeletal_mesh',{}).get('imported_size_cm')
        native_mesh_path=next((a.get('backend_path','') for a in ue.get('mesh_import',{}).get('artifacts',[]) if a.get('type')=='avatar'),'见原生 UE 验证记录')
        record={'name':j['name'],'slug':j['slug'],'clips':count,'bones':m['rig']['bone_count'],'original_bones':read(out/'rig_audit.json',{}).get('bone_count'),'fbx_roundtrip_current':fbx_current,'native_import_current':native_current,'native_size_cm':actual_size,'source_unchanged':source_same,'guidance_unchanged':guide_same,'skin_qa':m['rig']['skin_qa'],'review_required':gates,'videos':videos,'folder':str(out)}
        summary.append(record)
        inventory.append({**record,'skeletal_sha256':mesh_sha,'blend_sha256':sha(out/f"AS_{j['slug']}.blend"),'clip_inventory':clip_sha})
        speed_rows=['| 步态 | 每片段周期数 | 参考速度 cm/s | 循环秒数 |','|---|---:|---:|---:|']
        for c in m['clips']:
            if c.get('reference_speed_cm_s') and c['loop']:speed_rows.append(f"| {c['name']} | {c.get('gait_cycles',1)} | {c['reference_speed_cm_s']:.3f} | {c['duration']:.3f} |")
        handoff=f'''# {j['name']}（{j['slug']}）动作资产交付

本套含 **{count} 个已烘焙动作、{m['rig']['bone_count']} 根骨骼**。骨骼网格：{link(out/f"SK_{j['slug']}.fbx",'SK FBX')}；可编辑源文件：{link(out/f"AS_{j['slug']}.blend",'Blender 源文件')}；原指导：{link(Path(j['guidance']),'动作指导')}。

{NOTES[j['slug']]}

## 骨骼与蒙皮

所有动作与此物种专用 Skeleton 配对，不按相同骨骼数跨物种共用。固定 `root` 负责游戏坐标，陆地动物的原 `Root` 重命名为 `pelvis`，避免 UE 忽略大小写造成重名。骨骼角色、链、原坐标及蒙皮统计见 {link(out/'rig_update.json','骨骼更新清单')}。

{('此骨架已用自然站姿校准绑定方向，并重新计算所有动作的相对骨骼曲线。原网格、UV 和此次修订的蒙皮权重保持不变，落脚轨迹得到保留；地面动作另做接地和姿势修整。具体记录见 animation_manifest.json 中 reference_pose_calibration。' if m.get('reference_pose_calibration') else '此骨架使用专用的分段绑定与本物种动画曲线。')}

蒙皮未绑定顶点：{m['rig']['skin_qa']['unweighted_vertices']}；最大有效影响数：{m['rig']['skin_qa']['max_influences']}；权重和最大误差：{m['rig']['skin_qa']['max_sum_error']:.8f}。末端控制与 IK 已烘焙到变形骨，运行时不依赖 Blender 约束。

## 动作与审核

FBX 回读当前版本：{'通过' if fbx_current else '等待完成'}。UE 5.8.2 当前版本：{'通过' if native_current else '等待完成'}。检查记录分别为 {link(out/'fbx_roundtrip_qa.json','FBX 验证')} 和 {link(out/'ue_import_qa.json','原生 UE 验证')}。姿态抽样和慢/快步态连续视频提供视觉复核；没有将这些检查宣称为全部动作的发行品质认证。下表的“需细修”和 G 候选应保留禁用状态。

'''+ '\n'.join(clip_lines)+'''

## UE 接入

1. 先将 SK FBX 作为 Skeletal Mesh 导入并新建本物种 Skeleton，不从现有 SM 静态网格获取骨架。
2. 分动作 FBX 选择 Animation Only，并指定同一 Skeleton。片段内的微小绑定参考三角形只用于保存 FBX BindPose，禁止作为游戏网格导入。
3. FBX 内部坐标为厘米，导入 Uniform Scale = 1，Convert Scene Unit = true；关闭默认 30 FPS 重采样，按清单分别使用 30 或 60 FPS。可编辑 Blender 源场景为米，无需缩放源场景。
4. 关闭 Root Motion，由现有 Actor 移动逻辑拥有位置。移动速度用每帧 Actor 位移求得，不能假设 SetActorLocation 驱动的 Actor.GetVelocity 一定有效。按下方参考速度调整播放率，并在真实摄像机距离复核落脚；少年动物须同步处理 0.55 倍尺度。
5. 为 AActor 增加骨骼表现组件/AnimInstance，明确替换 Body 的显示、碰撞和引用，并确保新资产被 cook。推荐先单物种接入，检查转向混合、地面偏移与有限足部 IK，再扩展到其他动物。
6. 动作中没有伤害结算 Notify，当前即时伤害、存档 Id/Definition、捕获/跟随、奖励与掉落仍由原系统负责。G 动作、真实飞行和鱼类游泳 Actor 需要另行实现对应玩法时序。

'''+ '\n'.join(speed_rows)+f'''

隔离验证项目文件为 `E:/AiAgent/XLingGame/.animal-qa/HW/qa/pipeline/UEQA/UEQA.uproject`，本套网格原生路径：`{native_mesh_path}`。这是原生导入验证项目；当前游戏运行代码和桌面发行包尚未接入这些动作。
'''
        (out/'交付与接入说明.md').write_text(handoff,encoding='utf-8')
        video_html=''.join(f'<figure><video controls loop muted preload="metadata" poster="{href(out/"loop_preview"/Path(v["path"]).stem.replace("_10loops","")/"0001.png")}"><source src="{href(Path(v["path"]))}" type="video/mp4"></video><figcaption>{"慢速动作" if k==0 else "快速动作"} · {v["cycle_seconds"]:.3f} 秒/片段 · {v["native_fps"]} FPS</figcaption></figure>' for k,v in enumerate(videos))
        cards.append(f'<section class="animal" id="{j["slug"]}"><header><h2>{escaped(j["name"])}</h2><p>{count} 个动作 · {m["rig"]["bone_count"]} 根骨骼 · <a href="{href(out/"交付与接入说明.md")}">独立接入说明</a></p></header><p class="note">{escaped(NOTES[j["slug"]])}</p><div class="videos">{video_html}</div><details><summary>查看全部 {count} 个动作的开始 / 中间 / 结束姿态</summary>'+''.join(visual_frames)+'</details></section>')
        rows.append(f"| {j['name']} | {count} | {m['rig']['bone_count']} | {'通过' if fbx_current else '待完成'} / {'通过' if native_current else '待完成'} | {len(gates)} | {link(out/'交付与接入说明.md','独立说明')} |")
    if strict:
        assert total==303 and source_ok and native_ok and structural_ok and visual_count==303
    result={'schema':'hearthward.animal.motion.delivery.v1','animals':14,'clips':total,'native_current_verified_clips':native_total,'source_unchanged':source_ok,'all_fbx_current':structural_ok,'all_native_current':native_ok,'fresh_pose_reviews':visual_count,'numeric_review_flags_clips':flag_count,'cloud_downloaded_references':cloud_success,'cloud_confirmed_credits':credits,'animals_summary':summary}
    save(ROOT/'交付验收汇总.json',result);save(ROOT/'资产完整性清单.json',inventory)
    report=f'''# 14 种动物动作制作交付

已按十四份指导生成 **{total} 个分动作 FBX、14 个修订骨骼网格、14 份可编辑 Blender 源文件**，并提供 **28 段慢/快动作连续预览**和 **303 组开始/中间/结束姿态**。原始模型与十四份指导文件的 SHA-256 校验一致。

{native_total} 个当前版本动作通过 UE 5.8.2 原生导入、压缩姿态采样、骨骼名称、厘米尺寸、根骨单位缩放、固定根骨、时长、30/60 FPS 与循环首尾检查。FBX 当前版本结构回读：{'全部通过' if structural_ok else '验证中'}。这些是可导入的动作资产，当前游戏 AnimInstance、游戏行为接入、cook 与桌面包更新仍需要接入任务。

直接打开 {link(ROOT/'动物动作预览.html','动物动作预览')}，或从下表进入每种动物的独立资产和接入说明。

| 动物 | 动作数 | 修订骨骼数 | FBX / UE | 需复核片段 | 资产入口 |
|---|---:|---:|---|---:|---|
'''+ '\n'.join(rows)+f'''

## 本次制作与骨骼修正

动作采用物种分开的接触相位、分段躯干/颈部曲线、平滑进入与退出，以及尾部、耳部、鳍和触须次级运动。脚掌和蹄底保持接触姿态，循环首尾与固定根骨经过回读验证。全部运行所需曲线已烘焙到变形骨。

修正陆地动物躯干枢轴、肢链朝向和错误足部权重；狐狸新增三节尾骨并移除耳名骨对全身的误绑；两种鸟类各新增六根翼骨；三种鱼建立十二节躯干及鳍骨，鲶鱼另重绑六根触须骨；鳗鱼沿原 S 形鱼体建立二十四节骨链。不同物种分别保留 Skeleton。

雄鹿、野兔、山羊、猪、狼、公羊和赤狐另做自然站姿绑定校准，去掉原肢体参考方向与 IK 站姿之间的多余扭转。动作随之重新烘焙；山羊浏览觅食的颈部弯折，以及野兔、狼和赤狐的倒地折肢也经过调整。

## 明确保留的限制

本次有 **{flag_count} 个片段触发蒙皮拉伸等量化复核标记**，名单与实测值写在各物种说明、`fbx_roundtrip_qa.json` 和 `animation_manifest.json`。这些片段作为可编辑候选交付，不能标为已完成自然动作验收；后续应继续修整局部权重或姿势，再检查连续运动。

母鸡与雉鸡的 `FlightShortG` 仍为折翼网格的姿态候选，真实展翼需要翼面拓扑更新。狼的 `Howl` 只有抬头体态；可靠张口与音频尚未制作。闭口咬击按原指导的替代方案实现。狐狸的独立耳部转动尚未提供。指导中 G 优先级仍保留玩法接入门槛，导入成功不会自动建立飞行、泳动、跳跃或伤害时序。

目前视觉检查覆盖各片段开始/中间/结束和主要慢/快移动的连续视频；尚未对全部 303 段做人工逐帧认证，也没有验证桌面发行包内的运行效果。后续接入应复核真实镜头下的体积保持、地面与植物接触、转向混合和瞬时命中表现。

## 来源、费用与文件

原模型来自用户现有 Tripo 文件，交付动作由本地物种曲线和接触 IK 制作。另下载 {cloud_success} 份 Tripo 固定预设作为参考，已确认消耗 **{credits:g} credits**；山羊参考请求曾出现 SSL 响应异常，没有取得可核对的任务号，其是否另计费不能确认。API key 未写入交付清单。云预设原件 `cloud_motion.fbx` 没有替代这 303 段自制动作。

各物种文件夹包含 SK FBX、AS Blender、`clips/`、`animation_manifest.json`、`rig_update.json`、FBX/UE 验证记录、`review/`、`loop_preview/` 和独立说明。完整文件 SHA-256 见 {link(ROOT/'资产完整性清单.json','资产完整性清单')}；结果统计见 {link(ROOT/'交付验收汇总.json','验收汇总')}。

UE 实测在隔离项目 `E:/AiAgent/XLingGame/.animal-qa/HW/qa/pipeline/UEQA/UEQA.uproject`，目录为 `/Game/AnimalMotionCm` 和校准后的 `/Game/AnimalMotionBound`，各套实际路径见独立说明。生成与验证代码位于 `E:/AiAgent/XLingGame/GameFactory-3A/operators/gen_motion/funcs/animal_motion`，原生导入使用公开 UEClient 适配器。
'''
    (ROOT/'制作交付说明.md').write_text(report,encoding='utf-8')
    nav=''.join(f'<a href="#{j["slug"]}">{escaped(j["name"])}</a>' for j in jobs)
    page='''<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Hearthward · 动物动作预览</title><style>
    :root{color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:#151b20;color:#ecede9;font:16px/1.7 "Microsoft YaHei",system-ui,sans-serif}main{max-width:1140px;margin:auto;padding:36px 24px}h1{font-size:34px;line-height:1.3}h2{margin:0;font-size:28px}a{color:#dbc493}p{color:#b7c0c1}.lead{font-size:18px;max-width:840px}.top{border-bottom:1px solid #3c4549;padding-bottom:28px}nav{display:flex;flex-wrap:wrap;gap:8px;margin:24px 0}nav a{text-decoration:none;background:#283037;padding:5px 12px;border-radius:5px}input{display:block;width:100%;padding:12px;background:#242d33;border:1px solid #4a555e;border-radius:6px;color:white;font:inherit}.animal{margin:36px 0;padding:25px;border:1px solid #39434a;border-radius:10px;background:#1d252b;scroll-margin-top:20px}.animal header p{margin:2px 0 12px}.videos{display:grid;grid-template-columns:1fr 1fr;gap:18px}figure{margin:0}video{width:100%;aspect-ratio:4/3;background:#3c454a;border-radius:5px}figcaption{color:#adb9bf;font-size:14px;margin:8px 0 20px}.note{color:#c0c9c5}details{border-top:1px solid #3f4a52;padding:14px 0}summary{cursor:pointer}.action{margin-left:10px;font-size:14px}.frames{display:flex;gap:5px;margin-top:15px}.frames img{width:calc((100% - 10px)/3);height:auto;border-radius:3px}.badge{float:right;font-size:12px;color:#d3c297;border:1px solid #5b5748;border-radius:4px;padding:1px 7px}.flag{color:#edbe91}footer{padding:25px 0;border-top:1px solid #414b50}@media(max-width:720px){main{padding:24px 12px}.animal{padding:15px}.videos{grid-template-columns:1fr}.badge{float:none;margin-left:5px}h1{font-size:28px}}
    </style><main><header class="top"><p>HEARTHWARD · 动物动作库</p><h1>十四种动物，各自的动作与骨骼</h1>'''+f'<p class="lead">{total} 个动作 · 14 份骨骼与可编辑源文件 · 28 段主要步态连续预览。点击播放视频，展开动作查看开始、中间和结束姿态。</p><p>动作已做结构和导入检查；“需细修”与玩法候选仍保留在说明中。当前游戏包尚未接入这些动作。</p><a href="{href(ROOT/"制作交付说明.md")}">查看完整交付说明</a></header><nav>{nav}</nav><label for="search">筛选动作名称或用途</label><input id="search" placeholder="例如：吃草、倒地、Walk、Swim…">'+''.join(cards)+f'<footer>本地预览 · 两段主要移动片段各连续重复 10 次。陆地预览地面按参考速度相对移动，镜头跟随；片段边界处地面重新起始，交付动作仍为固定根骨。<br>完整动作与限制见 <a href="{href(ROOT/"制作交付说明.md")}">制作交付说明</a>。</footer></main>'+'''<script>document.getElementById('search').addEventListener('input',e=>{const q=e.target.value.toLowerCase();document.querySelectorAll('.action').forEach(a=>a.hidden=!a.dataset.search.toLowerCase().includes(q));document.querySelectorAll('.animal').forEach(a=>{a.hidden=!!q&&!Array.from(a.querySelectorAll('.action')).some(x=>!x.hidden);if(q)a.querySelector(':scope > details').open=true;});});</script></html>'''
    (ROOT/'动物动作预览.html').write_text(page,encoding='utf-8')
    print({'animals':14,'clips':total,'native_current':native_total,'numeric_flags':flag_count,'source_unchanged':source_ok,'strict':strict})

if __name__=='__main__':main()
