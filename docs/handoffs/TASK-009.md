# TASK-009 交接

## 实现与设计边界

已实现个人普通物品查看面板：五种物品按定义顺序列出持有数量和单重；零数量隐藏，空背包有提示，总负重直接读取008容器。
Tab按下切换界面，Started避免持续持键反复切换；bTriggerWhenPaused允许暂停中关闭。
打开使用引擎世界暂停，关闭只恢复自己发起的暂停；打开前已有暂停时保持原有状态。
世界时钟和五秒动作沿用既有暂停语义，无第二套计时或库存副本。
名称加入既有本地物品定义，没有变更存储格式、容量或重量规则；未涉及CT-001跨容器契约。

依据GDD v0.3 第4/7/14章和Q026/Q153。Tab是灰盒阶段临时绑定，R23正式键位、R09暂停设置例外仍OPEN。
没有修改原始设计、资产、配置或004任务。未实现装备、转交、使用、关键物品、存档等后续功能。

## 提交与流程

工作分支 `codex/TASK-009-inventory-panel`，基于已推送 `47207be2c199f0b2247c0f24cdbe090b873f45e9` 堆叠开发。
测试对应当前实现工作树，提交后补绑定完整实现SHA；未合并main。
用户授权实施、验证、提交和推送。Issue和独立评审未落实，流程状态继续Blocked，不自批Ready/Done。
内容资产未编辑，沿用TASK-003锁，无新增锁需求。

## 验证环境和结果

UE5.8.1 / Win64 / Development Editor；MSVC14.44.35228、Windows SDK10.0.22621.0。
引擎构建、原生测试、启动和关闭均经UEClient公开API，命令见 [构建和测试入口](../qa/BUILD_AND_TEST.md)。

- Editor增量构建通过，证据 [build.json](../qa/evidence/TASK-009/build.json)。
- `Hearthward.Inventory` 实际执行2项，2项Success、0失败，退出码0；[原生报告](../qa/evidence/TASK-009/automation-index.json)。
- 原生进程日志仍有先前任务已记录的13条启动期Condition failed；[结果摘要](../qa/evidence/TASK-009/automation-result.json)完整保留这些diagnostics，未将日志表述为全绿。
- 两轮真实PIE共36/36检查通过：[结果](../qa/evidence/TASK-009/pie-results.json)、[可复现脚本](../qa/evidence/TASK-009/verify_panel_pie.py)。覆盖开关与持键、世界时钟/动作/移动/视角冻结、恢复、外部暂停保留和新会话重置；恢复后满载实测315cm/s。
- 四张截图已逐张检查：[空背包](../qa/evidence/TASK-009/empty.png)、[混合23.35负重](../qa/evidence/TASK-009/mixed-paused.png)、[满载100](../qa/evidence/TASK-009/full-paused.png)、[关闭后计时恢复](../qa/evidence/TASK-009/closed-running.png)。中文、列对齐、两位小数、1540支箭及并列动作HUD显示正确。
- 仓库结构与本单声明路径检查通过，后者仅按用户授权核对当前任务声明，不构成基线审批认证；[范围记录](../qa/evidence/TASK-009/scope-check.json)。

脚本首次仅用墙钟等待，后台3FPS与HighResShot耗时导致释放帧尚未完成就再次按下。
增加帧等待证实暂停输入可关闭；初次8帧等待又让五秒动作在恢复检查前自然完成。
最终等待至少3次Slate回调并保留时间下限，游戏代码未为这些夹具问题改动；两次失败原始结果保留在本机本任务输出目录。
验证使用Enhanced Input动作注入，检查实际Tab映射；未模拟物理键盘。截图输出尺寸检查不代表物理窗口/DPI兼容性验证。
未运行打包或全量游戏验收。

## 接手操作

进入灰盒地图PIE，单击视口后按Tab查看/关闭背包。
可先用 `Hearthward.Inventory.Add wood 10` 等已有开发命令授予物品，`Hearthward.Action.Start` 启动五秒计时，再打开面板观察暂停。
空背包为新灰盒会话状态，不确定故事开局内容。暂停前提为当前单一界面；后续多界面嵌套或设置需另开契约任务。
