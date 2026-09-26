# Hearthward 项目状态

核对日期：2026-09-26。当前核对主干为 `origin/main@7ce8262bd8721dc59fb0703d365dab2e10340f89`，PR #46已合入043批准文档和044生存实现；以下旧审计与发行证据保留各自SHA。这里记录该主干可确认的实现和各版本已有证据；当前任务归属以任务分支快照、交接和调度者确认核对，GitHub Issue 可选。逐项来源与待办见 [TASK-041 基线与验收账本](planning/TASK-041-baseline-ledger.md)。

## 当前可运行范围

从 `L_Bootstrap` 的“新游戏”进入自然营地。玩家能采集树木、石头和草药，委托弟弟采集并交付，使用共享仓储，建工作台／床／篝火，制作、维修、休息、烹饪，以及保存和继续游戏。地图与任务日志页面有入口；自然图没有旧开发地标和正式敌人，现有任务／地点数据不能充当正式关卡。资源、设施和生存数值仍是 Demo 范围。[README 操作入口](../README.md#运行)和[发行记录](releases/demo-20260924/REPORT.md)列出可复现路线。

TASK-026 自然地图底座和营地、TASK-027 的早期主角动作、TASK-028 的房屋／陈设／临时石骨斧、TASK-029 AI NPC、TASK-030 Demo 交互、TASK-031 双角色与界面透显均已有主干成果。旧 AI 开发标签 027—042 与当前 canonical 编号有重号；按[编号映射](planning/TASK-041-baseline-ledger.md#2-编号映射)追溯。TASK-020 的用户验收已记录；不外推为新系统或全流程验收。

## 版本与验证

| 范围 | 已确认结果 | 仍未证明 |
|---|---|---|
| `origin/main@a4998b10da54f162def767fa8bb7e308594bd490` | 主干含上述玩法合并与 PR #44 的文档／校验规则；相对 `ee3f4c2` 无新玩法 | TASK-042／043 未在该 SHA 重跑 UE 构建、PIE、Shipping 或完整试玩，均 `NOT_RUN` |
| TASK-029 与 TASK-028/030 的开发组合 | [组合报告](qa/evidence/TASK-029/main-task028-integration-20260924/REPORT.md)记录 UE Editor Development、Python 31/31、原生 43/43、Demo 70/70、自然路线 51/51 和隔离资产路线 49/49 | 受测源码为当时集成分支；旧 UI／跟随／战斗用例不视作该组合已重跑；不冒充当前 main 验证 |
| TASK-031 分支 | [角色交接](handoffs/TASK-031.md)记录移动、双角色动作和场景透显的定向验证 | 完整动作质量、正式敌人遭遇、Owner 视觉签收 |
| Windows Demo 0.1.0 | 受测源码 `8b54550b5f9d7d01c9e9e0f7444826090667f3f5`；[报告](releases/demo-20260924/REPORT.md)记录 UE 5.8.2 Win64 Shipping、本机独立安装、随包模型和真实键鼠小闭环通过 | 第二台无开发环境机器、不同显卡／驱动、全剧情／全图／长时性能仍 `NOT_RUN` |

Demo 的构建基线为 `main@68e68d817c4d5a39bf43eb7f27863fc734d04aef`，发行修复源码之后经 PR #43 合入当前主干。发行包的通过结果只绑定 `8b54550` 及其本机实测路线。发行报告记录了当时 TASK-027/028 的四项 Issue／Reviewer 元数据报错；PR #44 已把可选 Issue 与审查阶段校验规则带入主干，不改写旧报告的历史结果。

## 设计和流程

- 当前设计入口为 [CURRENT](design/CURRENT.md)；R01—R25 的正式未决项见 [OPEN_QUESTIONS](design/OPEN_QUESTIONS.md)。已有局部 Demo 实现和设计定案分别登记。
- TASK-042 的 [DSGN-003](design/DSGN-003-first-release-slice.md)首版范围与救援切片方向已于本轮获Owner批准；此前草案经PR #45合入main@35ac870。本轮批准增量随043分支交付，R17／R25仅登记获准子范围，玩法与实测仍未完成。
- TASK-043 以 TASK-042 为父分支，交付 [DSGN-R01](design/DSGN-R01-world-time-persistence.md) 的 D1—D6 世界时间／生产／刷新／旅行／保存候选规则，以及 [CT-TASK-043](contracts/CT-TASK-043-world-time-persistence.md) 工程草案。Owner本轮修改确认睡眠8小时、击晕计入清敌、树木2日刷新及其余方案；昼夜按实际时刻显示、击晕等效击杀已获明确确认，不再苏醒；清敌、胜利、奖励和刷新同规则；044已将现有击晕接入击杀结算，完整刷新世界系统仍未实现。R01／R02／R03／R05／R19／R22记录获准子范围，整条仍OPEN。床的短时恢复、资源不刷新和现有传送不作为正式规则答案。
- TASK-004、026—031 的实现、集成、验证、流程与 Owner 体验分栏见[账本](planning/TASK-041-baseline-ledger.md#3-旧任务五栏账本)。选定路线、斧头握持、防具、双角色动作、真实自然遭遇、正式经济、长程伙伴体验和 Owner 验收仍需各任务收尾。
- 任务 JSON 中多个旧任务仍为 `Blocked` 或 `Active`，这些状态不表示相关实现不存在。Issue 可选；TASK-027/028 保持 `Active` 且 Reviewer 未指定，正式审查前仍需独立评审。TASK-041 的 Owner 与 Reviewer 均由用户指定为 `XLingyyy`，同人指定不构成独立审查，状态仍为 `Blocked`。[TASK-041 交接](handoffs/TASK-041.md)记录本次验证边界。
- Git LFS 锁、共享地图和公共配置仍按 [WORKFLOW](../WORKFLOW.md) 核对；本次 TASK-043 只改文档，无二进制、玩法或游戏配置改动。

## 2026-09-26 顺序纠正

043设计交付已收尾，D1—D6产品规则批准保持有效，流程Blocked仅表示尚无独立PR审查；后续数值配表不再列作043本单的阻塞。下一单为canonical TASK-044（原稿046）生存／倒地救援／环境伤害设计。052代码分支误提前开启，单独保留、未合并，不进入043／044基线。当前origin/main核对为35ac8706e50fe8b7cf099b3b73e803572d5a3ff0。

## TASK-044 本轮实施

Owner已批准D1—D6并明确“沿用044，开始实现游戏代码”。分支codex/TASK-044-survival-rules从043的b7d4df35763bca5dd0bb97ed9ab78eea4f68098a继续，范围协调提交0ef6b18a0f13436784c88ab9505a3f4e270a4dc3，未引入052。当前已实现共享生存状态、药品预留／半份、互救、失败入口和schema5生存快照；受测源码8f512a7dc4cf46e33d70c67c9294b6a2f3d8da77已通过Editor构建、原生18/18和PIE 23/23，详见本单验证报告。该实现已由PR #46合入main@7ce8262；运行证据仍绑定8f512a7，不声称重跑合并提交。

## TASK-045 当前工作

由主干7ce8262开始codex/TASK-045-combat-stealth。Owner批准D1—D6，D4改为统一3秒静音零耐力处决，并授权沿用045施工、提交推送。运行模块接入角色、动画、碰撞、感知和保存；自然地图正式敌人／区域、专用处决资产和装备绝对表尚未配置。实际验证见[045报告](qa/TASK-045/REPORT.md)，当前成果仅在任务分支，不声称已合入main。
