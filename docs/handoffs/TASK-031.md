# TASK-031 交接

2026-09-24。Owner：XLingyyy；基线 `be29573`，同步的 main 为 `86386b7`；任务分支 `codex/TASK-031-character-models`。用户授权双角色模型、骨骼和动作接入及提交推送，未授权合并 main。旧 AI 内部031记录归档于 TASK-029/internal-history。

主角和弟弟使用 Resource/Tripo 下用户新 ZIP，均为完整 61 骨骼。双方现在均使用各自包内八段动作。主角使用用户补传的带动画 ZIP，已替换此前借用弟弟动作的版本；双方 Walk/Run 去除 pelvis 水平前进趋势，修复模型循环跳回。角色高度分别 180/160 cm，贴图和 Skeletal Mesh 材质使用标记已配置。主角八类游戏动作、弟弟等待/移动/工作/攻击随真实状态切换。新动作、骨骼、材质、重定向工程与源 ZIP 一同提交，二进制走 Git LFS，资产锁保留给集成。

当前资产复测：连续行走/冲刺回归6/6、主角PIE26/26、自然地图弟弟22/22。动画补丁未改C++；推送前保留远端97c4338集成更新（含main4b0da61），重新通过Editor构建及上述三项PIE。原生3/3和工具31/31仍归属上一轮。弟弟真实取得、交付两份木材，主角真实采集、攻击及弟弟协攻伤害均通过。详见 [位移修复报告](../qa/evidence/TASK-031/inplace-fix/REPORT.md)，测试代码完整 SHA 见该目录 tested-revision.json。

运行：打开 L_Bootstrap，点击新游戏；WASD/Shift/空格移动，T 委托弟弟，Z 等待、X 跟随。自然地图无战斗敌人，战斗验证使用显式开发夹具。单独打开自然地图浏览模式只创建主角。

限制：当前使用胶囊碰撞，无 PhysicsAsset/布娃娃/布料模拟；石骨斧沿用远端TASK-028临时挂接，未与手部骨骼联动；未接接触IK或攀爬能力。开发资源块可能与动作穿插。未做本轮实际键鼠、Shipping、第二机器和长时间全地图性能验收。

用户本地 Hearthward.uproject 的 EngineAssociation 及 IDE 生成文件保留，不提交。仓库既有 TASK-027 元数据错误未修改。独立 Reviewer 和 Issue 未登记，任务流程状态按原约定保持 Blocked；实现和任务分支推送按用户授权完成，不代表已合入 main。

合并后测试完整 SHA 和证据见 `docs/qa/evidence/TASK-031/inplace-fix/merged/`；合并来源已在远端任务分支，不包含 Agent 合并 main 操作。
