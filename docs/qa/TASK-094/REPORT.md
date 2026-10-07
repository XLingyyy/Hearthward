# TASK-094 资产盘点与实际来源读取报告

日期：2026-10-07。任务状态：**Active，静态盘点与150包只读取证已交付**。Owner：XLingyyy。Reviewer／Owner最终视觉结论未代填。

实际工作区为 `G:/GameFactory/Hearthward`，当前执行分支为 `codex/TASK-084-103-iteration`，HEAD为 `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；该HEAD由本轮 `git rev-parse HEAD`读取。本轮包括未提交任务快照和其他Agent的并行改动，不能据这个HEAD断言全部工作区无改动。父Agent负责最终合并diff、公共检查和README同步。

授权来源：用户在当前Codex会话允许配置UE MCP并执行084—103，设计问题暂缓；本子范围由根Agent派发，只做094资产台账、来源／候选／样板资料。未编辑Content二进制、现有制作源、源码、地图或根README；本子Agent未启动／构建UE；根Agent另已通过公开UEClient执行下述NullRHI只读读取并停止自有Editor。未生成／购买／下载新资产，未提交／推送／合并／发布。

## 结果与适用范围

交付 [845行登记](../../assets/TASK-094/ASSET_REGISTER.csv)、[合并候选清单](../../assets/TASK-094/PACKAGES.json)、095—099五份独立候选、[44条旧源／导入对应](../../assets/TASK-094/SOURCE_CANDIDATES.json)、[样板与技术提案](../../assets/TASK-094/STYLE_REVIEW.md)。最终扫描只读201个生产 `.cpp/.h`、明确的数据／旧manifest及少量限定资产目录，没有扫描整个Content／制作源／Saved／.git，没有读取Save源码、Runtime和字体。

| 静态结果 | 数量／结论 |
|---|---|
| 台账行 | 845；是引用／依赖／需求条目数，不能换算美术完成率 |
| 磁盘存在 | 806（含2个入口地图、37张资源图与767个uasset） |
| 缺文件 | 28，均为现有54cue共用的未生产人声组 |
| 未定需求 | 10，射手／族人／野猪／炉／锻造／烹饪／盾／腿甲／箭袋／SFX；新包名未造填 |
| 初始源家族候选 | 自然Rebuild124包；原静态CSV保留候选状态，实际ImportData与依赖读取补充见下文派生表 |
| 无可用源映射 | 36条 `source_pairing=UNKNOWN`，有独立发行阻塞字段；不默许可发行 |
| 源与运行配置精确对应 | R3 14 mesh＋14 skeleton＋303动作，共331条；旧配对导入依赖另70条 |
| 精确源文件缺失 | 0（静态配对字段逐项核路径）；不证明源内容和UE设置正确 |
| 包写者冲突 | 767个候选uasset各一个写者，重复为0；全部 `selected_for_change=[]`、LFS锁未获取 |
| SHA／checksum | 本轮计算0个；CSV只保留既有manifest指纹，不声称重新完整性核验 |

完整统计见 [static-scan-results.json](static-scan-results.json)。目录列举与旧导入记录均有明确证据层，不等于当前正常游戏显示。入口地图被登记用于说明来源，未进入候选修改包清单；`.umap`和ExternalActors无修改授权。

角色源链将当前Hero／Brother的两个既有ZIP与后来TASK-004生成概念FBX分开，未靠文件名替换来源。动画精确clip窗仍须UE读取。077正式立面三包、028家具／房屋的源与材质／碰撞记录、070刀枪样品、R3物种模型／动作与依赖均能由表定位；源家族和同名候选按其实际证据强弱标记。

## 2026-10-07 实际UE只读取证

根Agent通过公开 `UEClient.runtime.launch_editor` 启动独立UE5.8.2 NullRHI／NoSound Editor，PID17424；复合入口同时读取动画与本单精确包，未启动模型。已归档 [launch.json](registry-sources-20261007-01/launch.json) 与 [stop.json](registry-sources-20261007-01/stop.json)，二者 `ok:true`；根Agent确认自有Editor已退出。原运行日志保留于 `.agent-local/qa/inspections-20261007-01/runtime.log`。启动入口为Bootstrap；本单inspector未加载World、进入PIE或写包，不能把NullRHI读取记为正常游戏／渲染验收。

原样归档 [asset-sources.json](registry-sources-20261007-01/asset-sources.json)，原结果时间 `2026-10-06T22:59:46.825022+00:00`。状态 `READ_COMPLETE_WITH_UNKNOWN_PROVENANCE`；150个精确包的Registry、直接dependencies和referencers均 `READ`，无API异常。原36个UNKNOWN关注条目为26个实际包与10个UNRESOLVED需求，加124个自然候选共150包；两个地图已含于150内。新增 [160行实际来源派生表](../../assets/TASK-094/ACTUAL_SOURCE_PAIRING.csv) 和 [summary.json](registry-sources-20261007-01/summary.json)，原845行CSV及许可字段未改。

| 实际读取层 | 结果与边界 |
|---|---|
| ImportData | 114包 `READ`，每包1条源；36包是材质／草类型／World等不适用类，count与files保持null。150包SourceFile tag均未提供；没有tag仍可实际读到ImportData，不能由tag空值推定无源。 |
| 自然124：自身导入源 | 97包（30 StaticMesh＋67 Texture2D）指向97条项目内PRESENT记录、96个独立art_source文件；两个GrassCards包共享同一glb。来源定位有当前ImportData和stat字节数，未读源内容／hash。 |
| 自然124：源目录细分 | TASK-026/Rebuild为40包／39文件，TASK-004/polyhaven为56包／56文件，TASK-004/sketchfab为1包／1文件。目录名称仅定位来源，不证明许可。 |
| 自然124：外部导入源 | `Meshes/SM_S1_GrassCards`仅返回外部 `GrassCards.glb` 文件名，`NOT_READ_OUTSIDE_ART_SOURCE`、bytes=null；未resolve／stat外部路径。 |
| 自然124：生成类26 | 23 Material＋1 MaterialInstanceConstant＋2 LandscapeGrassType无自身导入记录。其中25包的目标内直接依赖可定位PRESENT项目源；`GT_S1_Meadow`只指向上述外部草网格。材质依赖源属于旁证，不能当自身几何／作者证明。 |
| UNKNOWN实际26 | Demo四组16贴图实际ImportData只留Color／Metallic／Normal／Roughness.jpg外部文件名；四个Demo材质依赖这些贴图。其余四材质与两个World无适用ImportData。本组没有确认项目内源配对，10需求仍UNRESOLVED。 |
| 原候选路径差异 | 17个自然包实际ImportData路径与静态editable_sources候选不同，派生表逐项记录；例如SM_Rock实际源为TASK-026/Rebuild/sketchfab/中岩石/stone assets.obj，旧候选指向TASK-004的d_m.jpg。保留旧候选历史，避免把纹理候选误当网格源。 |

97个本地直接配对与25个依赖旁证分别缩小原家族候选范围；剩余外部草网格及其草类型缺项目内直接源定位。文件存在和字节数仅证明当前路径可读，未验证作者、生成输入权利、导入设置／源内容一致性或发行条款。150包原许可仍为75 `UNKNOWN`、72 `CC0_SOURCE_RECORDED_PACKAGE_PAIRING_PENDING`、3 `UNKNOWN_ORIGINAL_AUTHOR_URL_PER_ASSET_TERMS`；对应发行阻塞逐项原样保留。READ150不增加许可通过数量。派生表160行／150包／10需求、原845行分母、150行许可／发行阻塞与原CSV逐项一致性、JSON解析及UTF-8检查均通过；本单目标路径 `git diff --check`通过。

两入口map的直接hard／soft、game／editor包引用：Bootstrap为6条（2 Engine、3 Bootstrap材质、1 Script）；Wilds为1349条（1330 ExternalActors、15 ExternalObjects、1 HLOD、1 GameMode、1 Engine、1 Script）。两map与150目标的直接交集均为0。依据已捕获的目标referencers与Wilds直接ExternalActor依赖取交集，可确认33自然目标的已保存两边关系 `Map→ExternalActor→Asset`（30 StaticMesh、2 LandscapeGrassType、1 Material；逐个中间Actor见summary）。此推导未新增读取／递归／加载World，不能据另外91包无两边命中判断未使用，不能推定WorldPartition实时加载、C++动态生成或玩家实际可见。

## 执行与用例

使用上层 `.venv/Scripts/python.exe -X utf8`（CPU／HTTP工作环境）。源码／JSON显式UTF-8读取；LFS指针只读文件头64字节，字节数直接来自stat；初始静态扫描未加载UE二进制；后续inspector仅加载精确导入类读取ImportData，两个World不加载。各阶段计算资产hash均为0。

主要执行命令：

```text
G:/GameFactory/.venv/Scripts/python.exe -X utf8 Hearthward/docs/qa/TASK-094/scan_assets.py
```

首次运行暴露028历史manifest部分 `materials_textures` 为字符串，扫描器修正为仅展开字典字段并保留原未知值；早期动态 `SM_`／printf路径与地图后缀误报已根据当前源码形式纠正，没有登记为游戏缺陷。最终运行exit0，生成CSV／JSON解析和唯一写者检查通过；精确源缺失0，三个旧样图文件均存在。未为文档盘点创建原生游戏测试。

| 用例 | 本轮结果 | 真实覆盖及缺项 |
|---|---|---|
| T094-C01 引用配对 | PARTIAL | 150包当前Registry／依赖／referencers已实际读取；自然97包自身源、25包依赖源、33包地图两边保存引用得到补证。外部源、真实运行对象／双向可见性与完整来源仍有缺项，未写全项PASS。 |
| T094-C02 来源状态 | PASS（静态登记） | 来源不足用UNKNOWN、待补权利或未生产记录，并写发行阻塞；没有将Owner仓库同步／积分授权等同逐件发行许可。实际许可清障尚未完成。 |
| T094-C03 样板评审 | BLOCKED | 已直接目视070三图及077历史卧室图，写三类对照；070样图Owner签收仍 `NOTRUN`，最终视觉决策暂缓。 |
| T094-C04 包范围 | PASS（静态候选） | 精确uasset路径单写者、无地图范围、无整Content许可；选择变更项和LFS锁均未执行。候选不构成编辑授权。 |
| T094-C05 净空关联 | PASS（保护约束） | 全部涉及通行／战斗／工作位项标 `WAITING_084_092`，没有冻结或替换碰撞。实测净空NOT_RUN。 |
| T094-C06 存储规范 | PASS（静态） | 新交付仅本单docs/assets、docs/qa、handoff和任务元数据；原源／Content／Resources职责保留，没有移动或重复创建根资源目录。 |

本单UE只读元数据取证：已实际执行。正常游戏资产可见性／渲染、正常键鼠、Owner视觉、性能、二机和真人体验：`NOT_RUN`。公共 `validate_repo`、工具unittest、最终 `git diff --check`由父Agent对整批组合结果运行；本单不重跑全套游戏构建。范围基线必须含获批任务快照的真实提交，当前任务快照未提交，`--task TASK-094 --base`检查明确 `NOT_RUN`，没有改验证器。

## 暂缓与交接

- **设计待确认**：070具体人物／营地／地表样图，以及派生首件的Owner视觉审阅；077山地石堡方向已有确认，最终美术仍待审。具体新样板未获签收前，095—098批量风格替换暂缓；本轮没有生成新的互斥风格。
- **运行门槛**：本单未执行084 G0连续实玩和092路线净空正式实走；批次进展以其根Agent报告为准。150包读取提供来源／保存引用定位，不冻结门洞、楼梯、返营路、工作位碰撞；有关修改仍需对应真实测量。
- **来源门槛**：Sketchfab作者／原页面／逐资产条款缺失；现有记录只包含2026-09-20仓库同步授权。角色ZIP／knight61与装备逐件权利仍有缺项；Tripo输入图权利须补记录；Marble积分授权有记录，发行条款／具体来源对应仍须清障。人声保持既有UNPRODUCED和暂缓决定。
- **协作**：Rebuild `M_RockScan`由097唯一写者，096读取；028家具由098写者，096复用并协调。R3模型和303动作默认复用；任何单包编辑前选首件、补准确范围、检查LFS锁及未保存编辑器状态。

README由根Agent统一更新，本单README同步尚待根集成。静态清单、实际源派生表和完整逐包证据可以被下游读取；094整单不记完成，未取得Owner验收。提交／推送／合并／发布均未执行。
