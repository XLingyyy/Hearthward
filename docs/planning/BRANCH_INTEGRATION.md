# 分支整合清单

核对日期：2026-10-05；main 基线 `ac6a302ba24ea6531ccdaf880d83e9a03779c51e`。本轮整合使用 TASK-076，用户已明确授权提交、推送并更新 main。

| 来源 | 核对版本 | 当前关系 |
|---|---|---|
| origin/main | ac6a302b | 含 PR #60 的053—074玩法施工批次 |
| codex/TASK-053-traversal | e6615795 | 已在main，正式 TASK-053 为通行玩法 |
| codex/TASK-052-time-integration | a83d8793 | PR #58 已合入main |
| Hearthward-ui-fix | d2b9673b | 旧UI已合入main |
| codex/TASK-053-title-wheel | fa1828ed79d26518a5163f522ee0dbf124097209 | 新UI来源；由本次TASK-076纳入整合版本 |
| codex/TASK-075-repository-organization | main上的本地补丁 | 目录整理已带入TASK-076，原补丁有stash恢复点 |
| codex/TASK-076-ui-gameplay-integration | main + 新UI + 整理补丁 | 整合交付／验证，见[交接](../handoffs/TASK-076.md) |

## 冲突处理

两侧共同修改31个文件，初次合并有14个冲突文件。本轮逐项解决文本冲突，并处理自动合并无法识别的功能差异：

- 视觉采用新版标题、加载、菜单、背包、技能、日志和局部地图，保留main的公共玩法状态与存档规则。
- 世界日夜继续由统一时钟驱动，HUD使用 DisplayDay／MinuteOfDay；不回退到旧序章光照逻辑。
- 伙伴攀越保留无玩家Gameplay组件的执行路径及消耗规则；生存保留失败／倒地、治疗设施和回档epoch校验，同时接入新版手动进食计时与道具互斥。
- 加载期间记录当前页面输入状态，结束后恢复实际页面；放弃救援需要明确选择确认。
- 新版局部地图保留为默认视图，增加世界地图切换以保留探索雾、发现地点、任务指引、路标和传送。日志地图定位进入世界地图。
- 保留main的TASK-053任务单／交接；本轮UI整合和证据使用TASK-076。原分支的TASK-053是历史UI编号。

## 证据归属

UI分支相对共同基线有4,332个变更文件，其中4,235个为QA材料。本轮未将整份历史试跑目录复制进主干工作区，main已有QA记录保持；新的必要证据见[TASK-076报告](../qa/TASK-076/REPORT.md)。

原始UI源码、授权与历史结果固定在[原UI交接](https://github.com/XLingyyy/Hearthward/blob/fa1828ed79d26518a5163f522ee0dbf124097209/docs/handoffs/TASK-053.md)和[原UI报告目录](https://github.com/XLingyyy/Hearthward/tree/fa1828ed79d26518a5163f522ee0dbf124097209/docs/qa/TASK-053)。原标签、受测版本和路径不改写为当前验证。原分支没有删除、改写或强推。

## 后续协作

新任务先 fetch 并核对 main、活跃分支和任务编号。玩法与UI在自己的任务分支开发，共享输入、存档、时钟和UI接口先协调。本次整合验证后再按已有权限提交评审，由有权限的人合入main；不要从旧工作树直接覆盖当前目录。

文件存放见[目录说明](../REPOSITORY_LAYOUT.md)。Git祖先关系是已集成判断依据，远端PR网页状态未作为依据。
