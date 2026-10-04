# TASK-053 技能节点规整测试版（2026-10-03）

此报告仅绑定日志改造前的技能规整历史实现。当前测试工程结果见[日志报告](../journal-preview/REPORT.md)，旧指纹PASS不能替代当前源码。

四棵技能树已按原前置深度规整为四个对齐层级，统一纵向层高100和横向叶列间隔78（1672×941设计坐标）。连续单分支使用同一列；一分二的父节点位于两个子节点上方正中，分叉按子树宽度展开。根节点居各分支标题正下方，图标、等级、底部进度和说明保持清晰间距。节点位置从原前置关系计算，原内容表中散乱的美术坐标不再决定该测试页位置。

原技能标题、低对比双螺旋和炭灰背景保留，原有节点、说明、点数、学习、免费洗点和返回继续使用。

沿用同一Development测试工程及独立GUID档池；没有Shipping打包、桌面导入、提交、推送或公开发布。分支codex/TASK-053-title-wheel，真实基线4db5789184fe38e041d62a1e68c8517338ea0b01，任务Active供Owner分析。此前标题及双螺旋版本的完整历史记录见[skills-preview-v2/REPORT.md](../skills-preview-v2/REPORT.md)，旧PASS不替代本轮源码验证。

四分支的29个原节点和25条前置连线仍来自原内容表，100%—150%字号均保持树状加点。右侧显示原技能说明、真实等级、前置和成本；原Learn／ResetSkills、点数预算、等级上限、效果及存档规则未修改。背包三栏／四分类、HUD厚原色条、弟弟位置、约五秒任务通知和十米指引过滤继续保留。

双击[主界面UI测试.cmd](../../../../scripts/ui/主界面UI测试.cmd)，自然地图加载结束后按K查看技能，Tab查看背包，Esc返回。使用已有UE5.8.2／Python／GameFactory环境，每次启用独立GUID测试池。桌面“归火”仍运行0.2.0-preview.20261002.2。实际游戏截图：[默认技能页](../hud_e6c5aa33e199/skills-default.png)、[感知分支](../hud_e6c5aa33e199/skills-sense.png)、[学习后](../hud_e6c5aa33e199/skills-learned.png)；原生截图：[150%字号](../verify_176ac51ccf13/verify_176ac51ccf13-skills-text-150.png)、[720p](../verify_176ac51ccf13/verify_176ac51ccf13-skills-720p.png)、[16:10](../verify_176ac51ccf13/verify_176ac51ccf13-skills-16x10.png)、[超宽](../verify_176ac51ccf13/verify_176ac51ccf13-skills-ultrawide.png)。

## 当轮验证

- [skills_grid_build_r1](../skills_grid_build_r1/build_result.json)：UEClient公开API构建HearthwardEditor／Win64 Development成功，错误与诊断为空。
- [verify_176ac51ccf13](../verify_176ac51ccf13/report.json)：409/409界面检查、63张原生UI截图；技能100项覆盖标题／导航移除、旧导航区域无点击目标、双螺旋元素、全部节点命中与原说明、前置阻挡、原点数／学习／等级上限／洗点、连线及150%字号、720p、16:10、超宽显示。原标题、设置、存档、确认框高亮26项、HUD51项、背包80项及实际PIE延时装备检查仍通过。夹具还原Gameplay、库存与设置并确认保存节点数不变。独立技能记录见[skills-preview.json](../verify_176ac51ccf13/skills-preview.json)。
- [hud_e6c5aa33e199](../hud_e6c5aa33e199/report.json)：真实自然新游戏44/44检查、13张GameViewport截图，覆盖默认／感知／学习后技能页、原根与子技能学习、点数扣除及免费洗点返还。旧任务通知约五秒后消失，日志仍完整，返回HUD不重播；背包四分类仍可用。实际三维十米边界11/11，见[hint-range.json](../hud_e6c5aa33e199/hint-range.json)。学习事件回执依照原规则保留在可弃测试池。
- [hud_4cdd1f008b92](../hud_4cdd1f008b92/report.json)：实际Development游戏入口自然加载结束后连续三次窗口响应通过。
- 原生界面三个状态（默认、锁定说明、150%字号）的实际节点坐标核对通过：同层对齐、等层高／等叶列、六处分叉父节点居中、单分支直列、原25条前置连线端点、根节点对齐和节点／等级无重叠，见[grid-review.json](grid-review.json)。
- 三轮各28项配置／UI源码／DLL／原技能内容与成长规则指纹均匹配当前文件，验证自有进程均已停止，启动器恢复开发用户设置。绑定见[source-binding.json](source-binding.json)。
- 七张原生技能PNG完全不透明（alpha255）；默认、150%字号、720p、16:10、超宽和三张自然地图技能画面已视觉检查。规则排布在各视口和学习状态中均保持；左上标题与低对比双螺旋继续保留。见[opacity-review.json](opacity-review.json)与[visual-review.json](visual-review.json)。原背包组合、Gameplay／Progression及技能内容表保持原规则。
- 仓库、33/33工具测试、差异和本地授权范围检查的实际结果见[validation.json](validation.json)与[scope-audit.json](scope-audit.json)。正式基线批准快照检查仍单独记录失败：真实基线没有未提交的TASK-053批准快照，本地审计不替代该检查。
- 八项桌面EXE／UI、正式玩家档和用户设置与修改前SHA256一致；BUILD-INFO也保持一致。见[preservation-result.json](preservation-result.json)和[installed-version-preserved.json](installed-version-preserved.json)。

## 历史失败与验证边界

上一版的命名遮蔽构建错误、Python未反射调用和首轮启动未在120秒进入自然地图均保留在[历史报告](../skills-preview-v1/REPORT.md)。当时相同参数的新进程通过，首轮启动原因未确认；本轮未修改启动逻辑，新启动验证通过。本轮构建和三次运行均通过。

原生事件检查不等于Windows物理键鼠完整实玩，Owner视觉验收尚待进行。当前UI未做Shipping构建或桌面导入。前轮HUD／背包报告只绑定当轮源码；后续源码改变后需要新的构建与验证。
