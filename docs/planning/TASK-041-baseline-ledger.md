# TASK-041 基线、编号与验收账本

核对日期：2026-09-25。仓库：`XLingyyy/Hearthward`。本表的代码与文档基线是 `origin/main@ee3f4c2eede9f856c4589fd4b85b1b05c82a1b6b`。它记录截至该提交的事实，不自动把旧分支的测试提升为该提交的测试。

## 1. 版本和证据边界

| 对象 | 精确版本／结论 | 依据 |
|---|---|---|
| 当前审计主干 | `ee3f4c2eede9f856c4589fd4b85b1b05c82a1b6b`；包含发行修复 PR #43 | `git ls-remote origin refs/heads/main`、主干 first-parent 历史、[PR #43](https://github.com/XLingyyy/Hearthward/pull/43) |
| 近期主干集成 | PR #37（TASK-030）→ #38（TASK-028）→ #40（TASK-029 及所选历史 041/042 修正）→ #41/#42（TASK-031）→ #43（发行修复）已合入；这些是 PR 号 | 主干 first-parent 历史及对应 PR 页面 |
| Windows Demo 0.1.0 | 受测源码 `8b54550b5f9d7d01c9e9e0f7444826090667f3f5`；开发基线为 `main@68e68d817c4d5a39bf43eb7f27863fc734d04aef`；发行标签指向受测源码 | [发行报告](../releases/demo-20260924/REPORT.md)、[发布页](https://github.com/XLingyyy/Hearthward/releases/tag/v0.1.0-demo.20260924) |
| 发行范围内的实测 | UE 5.8.2 Win64 Shipping 构建、本机独立安装、随包本地模型、物理输入 10 份木材委托、建台、制箭、保存与安装版恢复通过 | [发行报告](../releases/demo-20260924/REPORT.md) 第 15—24 行；只适用于上述源码、本机和路线 |
| 当前主干 `ee3f4c2` | TASK-041 只做文档核对；UE 构建、PIE、Shipping 和完整试玩均 `NOT_RUN` | 本单[交接](../handoffs/TASK-041.md) |
| 跨硬件 | 第二台无开发环境机器、不同显卡／驱动兼容性 `NOT_RUN` | [发行报告](../releases/demo-20260924/REPORT.md) 第 32—34 行 |

主干合并只证明代码已进入 main。PR #40 前的集成验证、PR #38 后的组合验证、TASK-031 分支测试和 Demo Shipping 验证各自绑定不同源码。Epic 的[测试框架](https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine)也区分 Editor/PIE 等测试类型；发行构建的 Build、Cook、Package、Run 是不同阶段，见[官方构建说明](https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine)。本表逐项记载实际覆盖范围。

## 2. 编号映射

本地输入 `docs/Hearthward_TASK-043-076.md` 是以旧编号编写的 Backlog 规划稿。用户于 2026-09-25 明确其 **TASK-043 对应实际 TASK-041**。本次只登记 canonical TASK-041；后续规划条目按 `原稿编号 − 2` 解释，例如原稿 044→实际 042、原稿 076→实际 074。未来每项仍须分别建立任务单和授权，不能把规划稿的建议路径当成 `allowed_paths`。

| 标签／编号 | 当前含义 | 追溯方式 |
|---|---|---|
| canonical TASK-027 | 主角基础模型／动画接入；经 PR #35 进入 main，之后又有 TASK-031 双角色替换 | [任务单](../tasks/TASK-027.md)、[交接](../handoffs/TASK-027.md) |
| canonical TASK-028 | 3D 资产导入与应用；房屋、家具陈设和临时石骨斧显示经 PR #38 进入 main | [任务单](../tasks/TASK-028.md)、[交接](../handoffs/TASK-028.md) |
| canonical TASK-029 | AI NPC vNext 对外交付；原 AI 开发所用 027—040 标签和相关证据在此收口 | [任务单](../tasks/TASK-029.md)、[历史归档](../qa/evidence/TASK-029/internal-history/) |
| canonical TASK-030 | Demo 采集、家具和配方；早期 AI `TASK-030` 战斗记录另存于 TASK-029 历史归档 | [任务单](../tasks/TASK-030.md)、[交接](../handoffs/TASK-030.md) |
| canonical TASK-031 | 主角与弟弟新模型／动作及场景透显；早期 AI `TASK-031` 也只是历史标签 | [任务单](../tasks/TASK-031.md)、[交接](../handoffs/TASK-031.md) |
| 历史开发标签 TASK-032—040 | AI 恢复、Belief、主动表达、Episode、战术、习惯、Routine、组件化和上下文返工；旧 JSON/MD 保留作证据，不当作新的独立对外交付 | [TASK-029 历史归档](../qa/evidence/TASK-029/internal-history/)、[TASK-029 交接](../handoffs/TASK-029.md) |
| 历史集成标签 TASK-041/042 | 2026-09-23 工作树的 UI／交互、自然采集、攻击和跟随修正；已选择性进入 TASK-029 的 PR #40。主干没有这两个编号的 canonical JSON/MD | [双工作树集成报告](../qa/evidence/TASK-029/integrated-playable/REPORT.md)、[TASK-029 交接](../handoffs/TASK-029.md) |
| **本单 canonical TASK-041** | 当前基线、编号和验收账本；与上一行历史标签同号但来源不同 | [本单任务](../tasks/TASK-041.md)、[JSON](../tasks/TASK-041.json) |

`origin/main@ee3f4c2` 的 `docs/tasks/` 截至 TASK-040，没有 canonical TASK-041/042 文件。PR #41/#42 均为 TASK-031 的合并请求，与同号任务无关。旧文件、测试名和失败记录不重命名，引用旧 041/042 时必须写明“历史集成标签”。

## 3. 旧任务五栏账本

“验证”仅写证据覆盖到的版本；“流程”记录任务快照和真实登记；“Owner 体验”独立于实现、合并和自动化测试。任务 JSON 的 `Blocked` 不代表其代码不存在。

| 任务 | 实现 | 主干集成 | 已有验证及版本 | 流程 | Owner 体验与未收尾项 |
|---|---|---|---|---|---|
| [004](../tasks/TASK-004.md) 资产源 | 自然、家具、敌人、动物等源候选已入库；部分树木／地表、家具、房屋部件由其他任务接入 UE。源资产数量和许可证按逐件索引核对 | 素材经多个资产 PR 入主干；引擎应用属于 026/028/030/031 的成果 | [004交接](../handoffs/TASK-004.md)有文件、格式和来源检查；不能当作全部资产在 UE 可用 | JSON 为 Backlog；整单导入和来源维护未完成 | 缺失防具／工具、正式风格、逐件游戏表现与授权许可继续归 004；无整单 Owner 签收 |
| [026](../tasks/TASK-026.md) 自然地图 | 4.032 km World Partition 底座、新营地与 Bootstrap 入口、局部植被和资源接线存在 | PR #25、#32 的相应内容已合入；后续局部返工以 026 分支交接为准 | [026交接](../handoffs/TASK-026.md)含局部 PIE／实机进度；旧图 27 项和局部路线不能证明重建全图、长路线、流送或性能 | JSON 为 Blocked；Issue／Reviewer 缺项，地图与 LFS 锁须由资产写者复核 | 选定营地及通行路线的地形、碰撞、导航、流送和视觉未整体验收；远区扩林仍暂停 |
| [027](../tasks/TASK-027.md) 主角基础动画 | Tripo 主角和基础八段动作进入工程；后续 031 替换双角色资产／动作 | PR #35 已合入 | [027证据](../qa/evidence/TASK-027/README.md)为该任务分支 Editor/PIE；未构成当前双角色动作封版 | JSON 仍为 Active，Reviewer／Issue 为空 | 与 031 协调单一动画写者，继续修关节、脚部贴合、动作过渡及真人视觉验收 |
| [028](../tasks/TASK-028.md) 资产应用 | 营地房屋、床／箱／椅／提灯陈设和随装备状态显示的石骨斧已接；斧头尚未跟随手部骨骼 | PR #38 已合入 | [组合报告](../qa/evidence/TASK-029/main-task028-integration-20260924/REPORT.md)在集成分支验证资产路线 49/49，使用隔离材料注入；主干 `ee3f4c2` 未重跑 | JSON 为 Active，Reviewer／Issue 为空；新增资产锁需交接核实 | 石斧手部挂点、防具源与穿戴、房屋视觉／性能和 Owner 签收未完成；椅／提灯有陈设和灯光，没有专属交互 |
| [029](../tasks/TASK-029.md) AI NPC | 结构化任务、真实执行、认知／记忆、主动交流和受控协作已接自然营地；旧 AI 标签在此收口 | PR #40 已合入，包括历史 041/042 的所选修正 | [组合报告](../qa/evidence/TASK-029/main-task028-integration-20260924/REPORT.md)有 Editor、原生 43/43、Demo 70/70、自然路线 51/51；[发行报告](../releases/demo-20260924/REPORT.md)有受限真实模型路线；均非主干 `ee3f4c2` 全面复验 | JSON 为 Blocked，Issue／Reviewer 为空；真实模型原始理解与 UE 护栏结果须分开 | 任意自然语言、角色表达、长程协作及 Owner 使用体验未整体验收；自然图尚无正式敌人来证明实景战术闭环 |
| [030](../tasks/TASK-030.md) Demo 交互 | 自然树石草采集、四配方、床休息、篝火烹饪和存档已实现；资源暂不刷新 | PR #37 已合入；Demo 包含此循环 | [030报告](../qa/evidence/TASK-030/demo/REPORT.md)为开发分支 70/70 PIE；[发行报告](../releases/demo-20260924/REPORT.md)为 Shipping 安装版的选定路线 | JSON 为 Blocked，Issue／Reviewer 为空 | 保留 Demo 边界，回归采集／家具／配方，收尾键鼠、静态火焰与专用动画；正式经济另行设计 |
| [031](../tasks/TASK-031.md) 双角色 | 两名角色使用新骨架、材质和动作；背包／对话保留游戏场景可见 | PR #41、#42 已合入 | [031交接](../handoffs/TASK-031.md)记录分支移动 6/6、主角 26/26、弟弟 22/22 和透显 16/16；发行版的双角色显示只证明受测包路线 | JSON 为 Blocked，Issue／Reviewer 为空 | 角色姿态、跳跃手臂、斧头握持及 Owner 视觉验收待完成；与 027 协调动画写入 |

## 4. 当前入口和剩余风险

- 正常入口为 `L_Bootstrap`：自然营地可采集、委托、入库、建造、制作、休息／烹饪、保存恢复。地图 `M` 与日志 `J` 页面可打开；[输入处理](../../Source/Hearthward/UI/HearthwardScreenWidget.cpp)允许这两个键在 campaign 中进入页面。[玩法组件](../../Source/Hearthward/Gameplay/HearthwardGameplayComponent.cpp)在自然图不创建旧开发地标和敌人，因此 4 处旧地点、旧任务显示内容不能记为正式自然关卡。
- [房屋实现](../../Source/Hearthward/Building/HearthwardTask028CampHouse.cpp)已生成 `Chair`、`DoorLantern` 和门灯点光源；没有坐椅、携灯等专属玩法。工作台木桌、床、箱子及可建篝火的交互属于 TASK-030。
- 本单没有修改玩法或 R01—R25；Demo 数值、旧档覆盖、未来地图和正式生存经济继续按设计登记处理。[设计现行入口](../design/CURRENT.md)与[未决项](../design/OPEN_QUESTIONS.md)仍是设计权威。
- 当前 task JSON 中 TASK-027/028 缺 Reviewer 和 Issue URL；TASK-041 的 Owner 与 Reviewer 经用户 2026-09-25 指定均为 `XLingyyy`，真实 Issue 和独立审查仍未登记。`gh` 查询本机返回 401，无法验证受权限限制的实时归属；公开仓库页面可见当前公开 Issue 数为 0。不能伪造流程签收。未复核本轮实时 LFS 锁，本单无二进制修改。
