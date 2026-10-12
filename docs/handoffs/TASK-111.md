# TASK-111

Active，实现完成，待用户手感／外观验收。TASK-110 已提交推送并通过 [PR #75](https://github.com/XLingyyy/Hearthward/pull/75) 合入 main，合并提交 `7f56a2f5b42358b9d14c3ac04ab1cc19c29ca987`。

TASK-111 分支 `codex/TASK-111-navigation-combat` 已于 2026-10-12 获用户明确授权提交、推送并合并 main。范围：暂停空白区域焦点闪烁、实际场景俯视地图、玩家／弟弟方向标记、顶部罗盘、普通攻击时持续行走／冲刺及上半身动画分层。

Editor 编译、暂停／移动规则与处决原生回归、真实自然地图的渲染和移动挥斧验证通过。详细证据和边界见 [验证报告](../qa/TASK-111/REPORT.md)。没有更新 v0.3.0 独立包。

直接打开 `G:/GameFactory/Hearthward/Hearthward.uproject`，Play 后检查 Esc 暂停、M 地图、旋转镜头，以及 WASD／Shift 移动期间的轻／重攻击。

保留用户原有 `docs/归火Hearthward游戏设计文档_v0.3.docx` 删除；该文件未纳入本任务改动。
