# TASK-096｜石堡开场与材质技术调查

## 2026-10-08 近景增量

2026-10-08 近景增量：原创灰石/旧木受光材质、四张1024贴图与2700三角面卧室门框已接入，新增七个准确资产包。门框净宽280厘米、净高380厘米，仅装饰；原碰撞和导航不变。Editor构建及卧室/撤离净空原生1/1通过，正式地图隔离新档的三个游戏视口已检查。夜间卧室入口与回廊地面可辨，梁下和回廊门框背侧仍较暗；没有据离屏截图调亮正式灯光。完整火烟、连续路线、新版Cook与Owner视觉验收未完成，TASK-096保持Active。 证据见[nearfield/REPORT.md](nearfield/REPORT.md)。

以下为本单此前调查与验证记录；旧“待批准方向/暂缓制作/未改Content”的描述以本节更新为准，旧测试只保留其原版本信用。

状态：Active，2026-10-07。无设计依赖的正式资产/碰撞职责/生命周期调查已实施；近景材质与夜袭效果样板尚未 Owner 审定。工作树 `G:/GameFactory/Hearthward`、分支 `codex/TASK-084-103-iteration`、参考 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，含共享未提交改动。未提交、未推送、未发布；最终运行版本由主 Agent 绑定。

## 已执行与证据

运行 `../.venv/Scripts/python.exe -X utf8 docs/qa/TASK-095/audit_bindings.py` 生成 [TECHNICAL_AUDIT.json](TECHNICAL_AUDIT.json)。已核对 CampaignWorld 的 HometownFortress、WorldPresentation 的光照与关卡回调、PresentationComponent 的读档/结束清理及094正式立面来源。

- 石质山堡方向已有 TASK-077 于2026-10-05的 Owner 确认。无需重新询问石堡还是木堡；待审对象是近景真实材质、补面和火烟的首件实机样板。
- 正式立面仅3候选包：`SM_StoneholdFacade`、`M_StoneholdFacade_PBR`、`T_StoneholdFacade_base_color`，准确路径见 [094包范围](../../assets/TASK-094/TASK-096-PACKAGES.json)。原来源记录为 KHR_materials_unlit；当前 WorldClockTint 只能证明时钟着色路径，命名 PBR 和历史静帧都不能证明已具备近景局部灯光响应。其真实 UE 材质设置待运行查询。
- 立面和家具 NoCollision；实际阻挡来自原生 HometownFortress 分件。采用 [084路线 Manifest](../../planning/TASK-084-103/ROUTE_MANIFEST.json) 的真实寝室/门洞/走廊/台阶与兄弟路线，不重新使用历史失败几何。地面动态 Z、真实跟随与撤离仍需084/092运行证据；本单未冻结碰撞。
- WorldPresentation 注册/撤销 LevelAddedToWorld；PresentationComponent 的 BeginPlay/EndPlay 注册/移除 OnSnapshotRestored 并 StopFixedCue。保存恢复会停止旧固定语音并重置本地播放状态。已确认这些源码配对，未新增火烟或声光实例，重复进入/读档实机生命周期仍未验收。
- 准确候选包3个，`selected_for_change=[]`、锁未取得；`M_RockScan` 的写者属于097、096为消费者，028家具写者属于098，避免跨单重复写。

## 定向验证与验收前置

复用 `Hearthward.Hometown077.BedroomAndEscapeClearance`，本轮结果 Success（0 warning／0 error），条目见下节。它验证原生寝室/门洞/逃生净空的独立夹具，不能代替完整正式地形、兄弟实玩或材质 Owner 审核。092局部真实路线检查待主 Agent 执行；本单无新几何失败证据，不改门宽、坐标或台阶。

|验收项|本轮结果|已有调查与剩余前置|
|---|---|---|
|T096-C01 新档撤离|NOT_RUN|正式入口/出口坐标已采用084；需正常新档自然行走、兄弟跟随和任务推进。|
|T096-C02 近景几何|NOT_RUN|原生碰撞与装饰职责已分清；需正式场景连续路线和真实地面 Z。|
|T096-C03 昼夜材质|NOT_RUN|来源 Unlit/时钟着色已登记；需 UE 材质设置查询及昼/夜、局部火光的首件样板。|
|T096-C04 表现生命周期|NOT_RUN|关卡/存档回调配对已核对；需实际重复进入、读档、离场及实例数量对照。|
|T096-C05 可读舒适|NOT_RUN|需批准火烟样板后连续战斗/救援实机；不能用烟雾遮掩缺面。|
|T096-C06 原规则|NOT_RUN|本单没有规则或源码修改；需新档与相关互动回归，不能以未改代码替代运行。|
|T096-C07 独立包|NOT_RUN|3候选源/包可追溯；需依赖闭合和独立 Cook 加载。|

## 2026-10-07 实际定向原生结果

主 Agent 经公开UEClient构建实际成功后执行；[本单原始测试条目](NATIVE_REUSE.json)保留每条 entries、warnings、errors及设备，源报告为 `.agent-local/qa/TASK-085-099/native-first-20261007/index.json`（原报告时间 `2026.10.06-20.35.14`）。受测源码为参考HEAD上的共享未提交改动；此轮在092等待修复之前。范围 1/1 Success，warning 0，error 0。上述静态盘点生成时尚无运行结果，现由本节补充。

|实际过滤器|状态|Warnings|Errors|
|---|---|---|---|
|`Hearthward.Hometown077.BedroomAndEscapeClearance`|Success|0|0|

本单所选测试没有Warning。

警告原文保留，未把有警告的Success写为零警告；只按测试结果登记，不据警告推断正式输入或环境原因。这些是隔离原生行为证据，完整Cxx场景、正式地形、渲染/正常输入、Cook及Owner视觉仍为NOT_RUN。静态脚本编译、分单JSON/空选包/过滤器注册核对和定向 `git diff --check` 亦通过。

## 具体暂缓点与继续路径

Owner 仅需评审既定石堡方向下的近景材质/补面、火烟舒适度首件实机样板；正式通路和碰撞必须沿084/092已验路线。源资产的再分发授权由094登记，尚未明确条款的来源保留 UNKNOWN。

仍可执行净空夹具、现有材质只读属性查询和重复读档声光/VFX计数。允许共享源当前由092占用，待明确证据再开准确写窗口；本单未修改 CampaignWorld、WorldPresentation、PresentationComponent 或 Content。Owner 审样、完整路线、独立 Cook 均未完成，状态保持 Active。
