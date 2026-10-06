# TASK-077 石堡布局与 Marble 网格评估

最新进展：第二版裁出的103680面立面已接入实心主堡，采用保持源外观的无光照材质，并按游戏时钟调整昼夜颜色。原生背墙、基座和收边供给完整结构；卧室撤离路线16/16再次通过。Windows Demo已完成Shipping打包与独立启动、菜单、保存恢复检查，当前状态见 [发行记录](../../releases/demo-20261006/REPORT.md)。下文保留试导入阶段的问题定位过程，不代表仍在运行整份试验网格。

2026-10-06，受测工作区 `codex/TASK-077-hometown-stonehold`，基线 `53a1efd0ddf169a36265930aea63b58cabfaf061` 加未提交改动。此报告记录打包时受测工作区；随后用户授权将同一实现集成main。集成提交仅补授权与状态文档，未重新改动受测玩法和资产，最终美术仍未验收。

## 当前结果

| 项目 | 结果与证据 |
|---|---|
| UE 5.8.2 Development Editor | PASS，[build-result.json](build-result.json) |
| 卧室净空、门槛、回廊、侧门、模型及家乡回归 | PASS，4项，[native-results.json](native-results.json) |
| 实际自然地图新档、拾取、跟随、撤离 | PASS，16项，[walk-results.json](walk-results.json) |
| 灰石建筑、六栋石屋、主堡及卧室画面 | 已观察，布局阶段，[visual-results.json](visual-results.json) |
| Marble导入与材质绑定 | API操作通过，外观不接受，[import-mesh.json](import-mesh.json)、[bind-material.json](bind-material.json) |

构建、测试、导入、绑定和编辑器生命周期均通过公开UEClient执行。原生筛选 `Hearthward.Hometown077+Hearthward.Campaign049`。运行脚本 [verify_fortress_pie.py](verify_fortress_pie.py) 使用自然地图、正式标题页新游戏、UUID独立档池；未保存测试地图，未改用户旧档。

角色以AddMovementInput实际经过遗物、门口、回廊、楼梯、庭院、侧门和撤离点，路线不使用逐点传送。弟弟使用原导航跟随，最终进入occupied阶段。本轮新增房屋后重新跑完整路线。此项为引擎输入验证，不等同物理键鼠或真人通关；未录制完整试玩视频。

## 当前可见范围

约112×90米建筑群含卧室、回廊、楼梯、城墙、角塔、主堡和六栋石屋。石屋有坡屋顶、烟囱、门窗外观、灯笼，主堡增加扶壁；灰石与家具使用仓库已有资源。

**当前为可运行建筑布局。** 主堡、角塔和石屋主体是实心外观，尚无可探索内室；精细美术、街巷生活细节和新增夜袭火光、烟雾、动画未完成。保留现有夜袭流程，未声称完成新过场。

总览采用临时远景相机和日光，HUD仍显示夜间；正式开场时间未改动。

![当前布局，临时日光](overview-inspection-daylight.png)

![正式开场卧室](bedroom-beds.png)

## Marble检查与处置

Chisel世界生成中断且不支持Retry，转全景输入后生成成功。首版因悬空建筑与现代地面未接受；编辑全景后生成 [修正版世界](https://marble.worldlabs.ai/world/a59e2b3c-e9aa-432a-933e-0baf864f863a)。网页预览仅用于方向检查。

高质量GLB已下载：130352464字节、592899三角形、1材质、8192²内嵌PNG，容器检查见 [marble-mesh.json](marble-mesh.json)。此前导出标价3500积分，本次没有重复付费导出或购买积分/API，云端具体完成时间不可见。

裁出296794三角形进行试导入。初次日志确认材质缺少Nanite标记，使用了默认材质。公开绑定接口增加显式 `used_with_nanite=True`，读取确认标记保存；重启复查后该警告消失，但建筑仍严重碎裂、缺面，并有天空几何附着在主堡上。

![修复材质后仍不合格的实机画面](rejected-mesh-source-view.png)

原始GLB未经裁切的独立投影同样出现天空拉伸和碎裂。投影按三角形中心采样颜色，只用于几何定位，不能代表完整贴图渲染质量。

![原始网格辅助诊断](raw-mesh-projection.png)

结论：该网格未达到本场景可用标准。运行代码已撤下模型，试导入uasset隔离到被忽略的 `.agent-local/qa/TASK-077/rejected-content/`，原始GLB保留追溯。生成碰撞网格未用于游戏。官方也提示网格可能存在孔洞、浮片及天空拉伸：[Mesh export](https://docs.worldlabs.ai/marble/export/mesh)。

复现脚本、原文件、来源和公开适配器补丁见 [制作说明](../../../art_source/TASK-077/README.md)。框架新增项只涉及材质绑定3个文件，默认行为未变，实际引擎绑定与重启检查通过。

## 第二版网格对照（2026-10-06）

来源：[第二版世界](https://marble.worldlabs.ai/world/d7a06704-2d1c-4b9f-94db-37701016fbdc)。本轮本地处理未使用新的生成积分。原始带纹理网格594753三角形；顶点色网格16675739三角形。原始下载均保留。

顶点色版以5cm空间聚类降至2714312三角形，清理30米以上、地下及部分天空色面后导入。关闭投影仍有黑斑，说明自阴影不能解释全部异常。该试验存在降面影响，不能据此单独判定原始网格损坏。见 [降面记录](courtyard-v2-prepared.json)、[内部对照](courtyard-v2-vertex-interior.png)、[外部对照](courtyard-v2-vertex-exterior.png)。

带纹理版保持原始面与UV，仅烘焙节点变换并单独绑定原始贴图，未降面、未裁切。导入和绑定通过：[导入](courtyard-v2-import.json)、[绑定](courtyard-v2-material.json)。源材质明确声明 `KHR_materials_unlit`，见 [源材质记录](courtyard-v2-source-material.json)；按照 [Khronos规范](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_unlit/README.md)，此材质应忽略常规PBR光照。因此默认受光材质的黑斑不构成单独拒收依据。无光照视图下，石墙、拱门和木回廊能够清楚呈现。

![第二版原始贴图，无光照诊断视图](courtyard-v2-textured-unlit.png)

外侧仍能看到附着的天空表面和不完整建筑轮廓，见 [外侧无光照诊断](courtyard-v2-textured-exterior.png)。检查模型被临时抬高用于看清底部，截图中的悬空位置是测试摆放，不代表源模型地基高程错误。

**结论：保留第二版作为内庭美术制作基础，尚未通过整堡游戏接入验收。** 后续需要裁出可见立面、补全背面和屋顶、适配夜间与火光，并重新核验碰撞和路线。当前Unlit视图改变整个视口，仅用于诊断，不能作为日夜兼容材质或正式运行效果交付。试导入资产隔离在 `.agent-local/qa/TASK-077/courtyard-v2-evaluation-content/`；三条本轮LFS锁已释放。当前运行代码未引用试验资产。

本轮两次源格式检查均完成新游戏和卧室开场4项检查，见 [顶点色检查](courtyard-v2-vertex-inspection.json)、[纹理检查](courtyard-v2-textured-inspection.json)。这些检查不包括新网格碰撞、路线或视觉验收。游戏C++本轮未变动，先前4项原生、16项路线测试结果未被重复冒充为新模型验收。

## 仓库检查记录

前轮修复卧室与回廊100cm导航断缝、山坡基础和床碰撞块遮挡，见 [repair-evidence.json](repair-evidence.json)。本轮首次新增房屋构建因Rise遮蔽旧变量失败，改为RoofRise后通过；调查相机曾因暂停未更新视点，修正后重新观察四方向。

用户已授权将本轮成果提交推送到main；最终提交以Git记录为准。任务基线缺少TASK-077快照，任务基线模式校验前置条件不满足，未修改验证器；普通元数据检查见 [repository-checks.txt](repository-checks.txt)。主地图、UI、玩法数据和存档格式保持原样。
