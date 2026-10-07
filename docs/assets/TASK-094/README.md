# TASK-094 静态资产盘点入口

2026-10-07。当前为本地部分实施成果；风格样图Owner签收、G0实玩、路线净空和当前UE资产读取均未完成。

- [ASSET_REGISTER.csv](ASSET_REGISTER.csv)：845行逐资产／依赖／未定需求。`asset_id`是台账键，已有UE资产以包路径作键；`requirement:`和`native:`前缀仅说明盘点项，不注册新玩法ID。
- [PACKAGES.json](PACKAGES.json)：095—099精确候选路径、制作源及单包写者。对应五份 `TASK-095-PACKAGES.json` 至 `TASK-099-PACKAGES.json` 可独立交接。
- [SOURCE_CANDIDATES.json](SOURCE_CANDIDATES.json)：44条028源／旧导入事实，及生产源码中需动态解析的路径片段。明确不把尚未导入源和旧候选当当前使用。
- [STYLE_REVIEW.md](STYLE_REVIEW.md)：070三类样图、077石堡优先关系、Owner待审项、真实视角与技术预算提案。
- [DOWNSTREAM_DECISIONS.md](DOWNSTREAM_DECISIONS.md)：095—103具体待审选择、可独立实施子范围和工程条件，供root排程。
- [报告](../../qa/TASK-094/REPORT.md)、[静态结果](../../qa/TASK-094/static-scan-results.json)、[复跑脚本](../../qa/TASK-094/scan_assets.py)。

`STATIC_LITERAL_REFERENCE`／`STATIC_DATA_REFERENCE`证明当前文本里有引用；`SCOPED_PACKAGE_CANDIDATE`仅表示指定小目录里有文件；`HISTORICAL_*`保留原导入记录；均未证明本轮实机场景出现、二进制地图依赖闭包、LOD、碰撞或最终外观。

制作源配对分为 `EXACT_*`、历史家族／文件名候选及 `UNKNOWN`。源目录候选和相似文件名不能替代精确映射。CSV保留已存在manifest指纹，没有重新计算资产SHA。记录文件名、是否存在、字节数和LFS指针状态；当前被盘点的包无LFS指针。28个缺文件为既有未生产人声组，10条资产需求仍 `UNRESOLVED`。

候选清单是读取与派单资料。所有 `selected_for_change` 仍为空；编辑前须选择首件，补所属任务准确路径并取对应LFS锁。R3模型／303动作默认复用，目录列举没有给全部资产变更许可。没有 `.umap` 或ExternalActors修改范围；门洞／台阶／战斗区／工位碰撞不得在084／092净空完成前替换。

共享规则：`M_RockScan`仅由097提议修改，096为读取者；028家具由098负责，096复用并协调，不可由两个Agent同时改包。新角色、专用射手／族人、野猪、炉／锻台／锅、盾／腿甲／箭袋和SFX包名尚未定，不预造新Content路径。人声按既有用户决定暂缓。
