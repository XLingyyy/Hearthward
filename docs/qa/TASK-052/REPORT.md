# TASK-052｜实现与验证记录

2026-10-03。实现与自动化验证完成。分支`codex/TASK-052-time-integration`，调查基线`4db5789184fe38e041d62a1e68c8517338ea0b01`，Owner批准范围登记提交`275124aa0dd53314b2107c28449ec77bca60903c`。受测源码提交：`0bc1c3ee5d09cfca3f675cf657b6981f874f9138`。构建、原生和PIE运行后冻结源码并提交；其后只更新文档和证据绑定，未改变运行代码。Owner／Reviewer：XLingyyy，无Issue。任务保持Active等待Owner体验及正式评审，成果尚未集成main。

## 实现

WorldClock统一A有效实玩秒、W累计游戏分钟与各领域结算。新档第1日20:00，日期、HUD、感知、动物时段与光照读取同一时刻；06:00／18:00各有30分钟平滑过渡。暂停、加载和恢复冻结。真实床睡480分钟、篝火等待60／240／480分钟，校验设施及距离、Campaign、epoch、OperationId和StartW，回执包含实际推进量；重复请求不重复结算。

生存预演与提交共享实际技能和恢复参数，世界在最早失败边界停止。生产复用真实材料、劳动力和批次；睡眠不计兄弟劳动。资源沿既有身份，仅耗尽排期，阻塞保留原due，到期恢复一份容量。野外敌人采用下一个全局四日边界；跨周期无多次复活、活敌回血或无人击杀收益，故乡及增援保持永久终态。

旅行先准备流送地形和导航，验证全部必随落点后提交位置、短动作中断及本次参战活敌回血。倒地参战弟弟随行且保留倒计时，独立弟弟留守并保留地形流送源；盔甲、异常、警戒、击晕与死亡保持。落点失败不提交位置／敌血。

schema9/HWS9保存ClockVersion和初始日／分钟。8→9保留W和旧显示基准，迁移可解释的旧野外due；缺失字段按实际序列化内容检测。新版字段套用旧封装时拒绝并保留原件。沿用备份、兼容预览及新版进度隔离；恢复安装完整快照、更新epoch并废止旧请求。恢复完成通知发生在战役安装后，避免回调看到混合时间线。

本次还修复自然路线验证中确认的主菜单Enter失效与确认框焦点问题，并按已集成051的原声明补回main遗漏的设置字段。没有修改Content、地图、引擎配置、051资料或制作录音。

## 工作流要求映射

| 要求 | 本单结果 |
|---|---|
| T-002 范围 | 批准登记提交为真实基线，仓库／路径检查PASS，见scope-validation.txt |
| T-024 共享契约 | Owner已批准CT-TASK-052 v0.2及提供／消费路径；公共结构和调用方当前构建PASS。正式人工代码评审待Owner |
| T-016 睡眠经济 | 定向PASS：实际生产／生存原生，真实床与篝火PIE；完整生态长跳时及真人路线PARTIAL，见矩阵 |
| T-017 战斗传送 | 定向PASS：三种流送旅行、零跳时、参战敌血范围、倒地随行和失败原子性 |

## 当前验证

| 层次／检查 | 结果 | 证据 |
|---|---|---|
| L0 工具自测 | 33/33 PASS，退出0 | [tools-validation.txt](tools-validation.txt)；工具源码未改动 |
| L0 仓库及正式T-002 | PASS：批准基线275124a，53份任务快照、83条改变路径、0错误、退出0 | [scope-validation.txt](scope-validation.txt) |
| L1 Editor Development | PASS，退出0 | [build.json](build.json)、[run_build.py](run_build.py) |
| L2 Time／Save／Camp／Nature／Campaign／Survival | 34/34 PASS，测试警告0、失败0 | [final-native.json](final-native.json)、[原始索引](native-index.json)、[run_native.py](run_native.py) |
| L3 渲染自然图 | 82/82 PASS，启动器退出0 | [pie-results.json](pie-results.json)、[launch.json](launch.json)、[验证脚本](verify_time_pie.py) |
| 主菜单按键回归 | Down、Down、Enter、Enter进入新进度 | [keyboard-final.json](keyboard-final.json)，由非Shipping夹具调用实际NativeOnKeyDown |
| 正常旅行＋倒地弟弟 | 保留45秒，A/W冻结，指定活敌回血，倒地不能保存 | [travel-down.json](travel-down.json)及PIE检查 |
| 独立弟弟留守 | 玩家到新站点，弟弟原位置保持 | [travel-stay.json](travel-stay.json) |
| 无地面落点 | 玩家／弟弟位置、敌血均保持 | [travel-blocked.json](travel-blocked.json) |
| 测试进程收尾 | 同一UEClient关闭其启动的编辑器，退出0 | [stop.json](stop.json) |

PIE从标题页进入正式自然地图，完成序章交互及移动输入，使用真实5秒建造、真实床连续睡眠和真实篝火请求，再进行暂停、落盘存档、反复读档及旅行。建造前只补充夹具材料；旅行在隔离GUID档池用非Shipping命令设置参战／倒地和无地面目标，实际生产Travel、导航和流送路径运行。每次旅行前由实际移动回到有效营地站点。82项包括状态断言及UI截图生成检查，未将其表述为82项独立真人测试。

最终渲染档池：`194fceb3-00fc-41d4-99a8-1be98a10bbb9`。原生档池由run_native.py的标签final固定派生。正式玩家档池未使用。场景截图为[序章夜景](prologue-night.png)、[睡眠后夜景](after-sleep-night.png)、[白天设施菜单](campfire-menu.png)；HUD日期见[白天UI](camp-daylight-ui.png)和[恢复后UI](restored-night-ui.png)。UI捕获单独渲染部件，黑底不代表场景；白天世界可见于设施菜单截图边缘。日／夜和1／4／8小时设施选项已实际检查，06:00／18:00连续渲染路线未执行。

环境：Windows 11 24H2、UE5.8.2、Development Editor、RTX4060 Laptop、工程`G:/GameFactory/Hearthward/.agent-local/task051`。全部构建、原生、启动和关闭均使用GameFactory公开UEClient。可复现命令从`G:/GameFactory`运行：

```powershell
& .venv/Scripts/python.exe -X utf8 Hearthward/.agent-local/task051/docs/qa/TASK-052/run_build.py
& .venv/Scripts/python.exe -X utf8 Hearthward/.agent-local/task051/docs/qa/TASK-052/run_native.py --label final
& .venv/Scripts/python.exe -X utf8 Hearthward/.agent-local/task051/docs/qa/TASK-052/launch.py
```

原生使用NullRHI／NoSound；PIE实际渲染、NoSound。自动化索引测试警告为0，但引擎扫描仍报告8个无关动物测试／Demo包保留LFS指针，以及Trace Server警告；详细诊断保留在原始结果，未隐去。这些包未被本次自然路线引用。生产所需471个动物资产已从既有LFS对象恢复，未产生资产差异，见[本地资产记录](local-assets.json)。

## 修复前复现及调试记录

- 基线编译因main遗漏051设置声明失败：[baseline-build-failed.json](baseline-build-failed.json)。按原已集成声明补齐后最终构建通过。
- 资源恰好due未恢复、生存预演遗漏饥饿技能倍率均先复现为FAIL：[baseline-native.json](baseline-native.json)、[夹具差异](baseline-source.json)。当前对应测试已通过。
- HWS8包裹schema9字段被误当旧档接受，先复现后严格拒绝：[envelope-baseline-native.json](envelope-baseline-native.json)。原Compatibility测试自身使用新字段＋旧头，已改为正确当前头HWS9，保留原兼容预览断言；[pre-final-native.json](pre-final-native.json)记录中间失败，最终34项通过。
- 主菜单按键先复现未进入新档：[keyboard-baseline.json](keyboard-baseline.json)、[keyboard-baseline-pie.json](keyboard-baseline-pie.json)。当前相同按键序列通过。
- [pie-attempt1.json](pie-attempt1.json)至[pie-attempt5.json](pie-attempt5.json)保留夹具调试失败：走位穿过篝火、Guid字符串含对象地址导致选错存档、旅行前不在真实站点、Python读取受保护字段。最终修正为稳定Guid数值、真实走位和引擎结构体export_text读取；未删失败或降级断言。

旧编辑器占用已由Owner处理；启动器现保留同一UEClient到验证结束，并在finally关闭自己启动的进程。旧052、051、动物及Demo历史PASS未替代本单结果。

## 限制及交付边界

[MATRIX](MATRIX.md)逐项记录PASS／PARTIAL和NOT_RUN。真实建筑阻挡／拆除与World Partition身份重入、所有资源白名单逐点、野生刷新保护解除、完整成长／繁殖长跳时、回调重入与迟到模型注入等未穷尽验证。物理键鼠因桌面捕获与实际窗口不符停止，当前使用引擎合成按键和移动输入；真人完整路线、视频、Shipping、十小时、第二机器和人声NOT_RUN。Owner与Reviewer同为本人，自动化通过不代替其正式验收。

源码与证据按已给授权提交推送任务分支。未创建Issue／PR、未合并main或发布。固定录音按Owner要求继续暂缓。
