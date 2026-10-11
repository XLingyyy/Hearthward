# TASK-105 工程优化计划

2026-10-11。基线 `ff5f6f985b326011b9dc968ebe9ace55f4582088`。当前用户批准先推进 M0 工程优化。本地分支 `codex/TASK-105-engineering-optimization` 已整合补丁，并修复 UE 5.8.2 JSON 字段键转换、OBJ 检出换行、旧版本测试夹具及文档断链。当前验证见 [本单报告](../../qa/TASK-105/REPORT.md)。包内 GitHub 403 是上游准备阶段记录，本轮未尝试推送、合并或发布。M1–M3 保留内容规划。

## 本次已准备的生产代码

1. `Gameplay/HearthwardCatalogIndex.h`：不可变目录的内部共享索引。保留FString键、原始行对象身份、首次匹配的重复项语义。非ID数组、缺id或类型错误的表不索引，由原函数处理，不能变成静默miss。查找本身不写索引，不依赖世界、存档或timeline。
2. `HearthwardGameData.cpp`：一般目录Find走索引快路；Catalog加载与公开函数签名不变。
3. `HearthwardCampaignState.cpp`：战役Find走同一索引实现；一次转换FName为FString。Total/Cleared直接计数，不再复制完整敌人结构数组；仍每次读取当前Enemies，未增加易失效的计数缓存。
4. `scripts/qa/validate_catalog_ids.py`：标准库只读数据检查，覆盖12张已有ID表的缺项、类型、空ID/重复ID，以及JSON重复键/非有限数值。不是完整经济Schema或玩法验证，不修改目录。

## 成本与契约

索引从每次扫描整表改成基于TMap的键查找，增加一次初始化工作与常驻键/共享引用内存。目录顶层成员和id在进程内必须不变；目前Catalog只加载一次，本次未新增热重载。如果将来支持运行时改表/改id，必须在同一变更中设计索引重建，不能复用旧索引。其他行字段仍通过原对象读取。没有GPU、UI或帧率提升实测。

计数改动不改变base/reinforcement范围、Health<=0、80/84/88分母、严格>95%展示条件或胜利规则。未知组仍排除；读档/增援/伤害后直接反映当前数组。保存结构未变。

## 后续顺序

|包|工作|先决条件|验收与风险|
|---|---|---|---|
|E-01|整合此补丁并执行原生/旧回归|当前主干精确匹配或经人工rebase；UE编辑器已关闭|7项新用例及已有战役/装备/存档；无实际编译不合并|
|E-02|运行正式目录数据检查|完整源码克隆|12张表实际结果；不得自动修复ID或放宽失败|
|E-03|UI失效/初始化定位|可记录正常玩家操作|枚举OnChanged订阅、HUD动态状态和图片加载；先测量，不直接删广播|
|E-04|六场景双后端联合性能|固定Shipping包/硬件/画质/模型身份|完整帧样本、冷暖分开、原门槛；定位后才选择CPU、GPU或流送优化|
|E-05|语言质量与时延|原60题冻结；独立同义表达保留集|理解/确认/执行分开；不改安全校验、默认部署或数量边界冒充优化|
|E-06|资产/纹理/UI预生成/Cook|准确资产来源、锁和批准首件|同机位视觉、实际Cook/Load/往返；本次不改二进制|
|E-07|同版证据聚合与UE构建自动化|真实可用UEClient环境及受控runner|记录源码/包/配置/设备身份；现有repo-policy通过不能当UE通过|

## 本次有意不做

不删除Gameplay的OnChanged广播：它可能承担生命/耐力/伙伴等动态显示更新，缺少完整订阅和实际Paint证据。不同UI页的缓存和首次图片导入需实际测量。

不降Nanite/阴影/分辨率，不替换模型或放宽质量目标，不重构存档，不改变获批战斗与人口规则，不凭高面数认定某资产为瓶颈，不触碰Content/Runtime/依赖锁/工作流权限。

## 复核入口

见[本单报告](../../qa/TASK-105/REPORT.md)与[测试矩阵](../../qa/TASK-105/TEST_MATRIX.md)。可执行入口：

```text
python scripts/qa/validate_catalog_ids.py --json
python -m unittest discover -s scripts/tests -p test_catalog_ids.py -v
python scripts/validate_repo.py
python scripts/validate_repo.py --task TASK-105 --base ff5f6f985b326011b9dc968ebe9ace55f4582088
python -m unittest discover -s scripts/tests -v
```

UE构建与测试使用项目现有 `docs/qa/BUILD_AND_TEST.md` 中的UEClient入口；新增过滤器 `Hearthward.Engineering.Task105.` 应发现7项，不写一个自动通过的假运行器。保留正常Shipping与完整库存/伙伴/双营地读档验证。
