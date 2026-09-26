# Hearthward 项目状态

核对日期：2026-09-27。当前核对主干为 `origin/main@e95fde185dc38d0b69c2de42ce66db0d1fd93294`，PR #46已合入043批准文档和044生存实现，PR #47已合入045战斗实现，PR #48已合入046营地经济，PR #49已合入047成长装备；以下旧审计与发行证据保留各自SHA。这里记录该主干可确认的实现和各版本已有证据；当前任务归属以任务分支快照、交接和调度者确认核对，GitHub Issue 可选。逐项来源与待办见 [TASK-041 基线与验收账本](planning/TASK-041-baseline-ledger.md)。

## 当前可运行范围

从 `L_Bootstrap` 的“新游戏”进入自然营地。玩家能采集树木、石头和草药，委托弟弟采集并交付，使用共享仓储，建工作台／床／篝火，制作、维修、休息、烹饪，以及保存和继续游戏。地图与任务日志页面有入口；自然图没有旧开发地标和正式敌人，现有任务／地点数据不能充当正式关卡。043—047已批准规则和运行表见各任务报告；正式物种、源点和任务关卡仍有内容缺口。[README 操作入口](../README.md#运行)和[发行记录](releases/demo-20260924/REPORT.md)列出可复现路线。

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

## 顺序与历史纠正

043产品规则已批准，044／045实现已合入main。当前按canonical048推进，原稿050对应本单。此前误开的052独立保留，没有引入当前基线。

## TASK-044 本轮实施

Owner已批准D1—D6并明确“沿用044，开始实现游戏代码”。分支codex/TASK-044-survival-rules从043的b7d4df35763bca5dd0bb97ed9ab78eea4f68098a继续，范围协调提交0ef6b18a0f13436784c88ab9505a3f4e270a4dc3，未引入052。当前已实现共享生存状态、药品预留／半份、互救、失败入口和schema5生存快照；受测源码8f512a7dc4cf46e33d70c67c9294b6a2f3d8da77已通过Editor构建、原生18/18和PIE 23/23，详见本单验证报告。该实现已由PR #46合入main@7ce8262；运行证据仍绑定8f512a7，不声称重跑合并提交。

## TASK-045 已集成

由主干7ce8262开始codex/TASK-045-combat-stealth。Owner批准D1—D6，D4改为统一3秒静音零耐力处决，并授权沿用045施工、提交推送。运行模块接入角色、动画、碰撞、感知和保存；自然地图正式敌人／区域、专用处决资产和装备绝对表尚未配置。实际验证见[045报告](qa/TASK-045/REPORT.md)，成果已由PR #47合入main；测试仍绑定报告所列源码。

## TASK-046 实施

Owner明确批准D1—D6并要求沿046施工、提交推送。分支`codex/TASK-046-camp-economy`已同步main@2dab1f8；工作目录与TASK-027隔离。实现营地1—8阶、四类设施I—III、共享人口／口粮／仓储、独立设施队列、五身体岗位、兄弟真实在岗工效、有限源点、建造预留和拆迁账本，新增四页营地管理及同档恢复。详见[046报告](qa/TASK-046/REPORT.md)。

正式自然食物点、营救／故乡关卡、高级装备／菜单仍由后续任务配表；046不新增地图或二进制资产，普通族人为生产岗位状态，未新增个体模型／作息。旧档建筑拆返账本按零迁移。已由PR #48合入main；合并不改变046原测试SHA，Owner自然图体验验收另记。

## TASK-047 实施与验证

Owner已批准D1—D6。独立分支codex/TASK-047-equipment-recipes基于main dbba6d0，实现60级成长、29技能、62物品、49配方、38件耐久装备、双方容量升级及schema6。运行配置已替换旧原型语义，候选JSON保留批准时记录。Editor构建、原生32/32、真实PIE64/64与工具33/33已通过；实现已由PR #49合入main，原验证绑定原源码，结果见[047报告](qa/TASK-047/REPORT.md)。自然资源点与正式任务奖励仍归048／049，047已集成，048内容提案尚未批准。

## TASK-048 当前设计阶段

由上述主干建立codex/TASK-048-nature-content，仅修改文档及候选数据。8野生＋3家畜＋4鱼、资源刷新白名单、作物返种、饲料繁殖和钓鱼宝藏形成D1—D6提案；Owner尚未批准。38项关系检查通过，有限来源模型沿046优先采空较少余量点。动物资产适配、实际自然布点和UE验证均NOT_RUN。见[设计](design/DSGN-R15-nature-production.md)和[报告](qa/TASK-048/REPORT.md)。
