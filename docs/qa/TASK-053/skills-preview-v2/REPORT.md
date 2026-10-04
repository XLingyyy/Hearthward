# 历史记录：技能页标题与双螺旋版本

本文件仅绑定verify_314d25bd287f／hud_caa4d34d66ae／hud_3e8bfdbfe228时的源码。当前节点排布版本见[当前报告](../skills-preview/REPORT.md)。

# TASK-053 技能界面细化测试版（2026-10-03）

当前技能页已移除顶部地图／装备／技能／任务／图鉴／系统六项导航和左上归火标志，原标志位置改为“技能”。旧圆弧刻纹已替换成右下低对比的反向双螺旋印记，两条交错螺旋配有横向连接，置于炭灰雾面背景及面板后方。原有树、节点、说明、技能点、学习、免费洗点和返回仍可用。

沿用同一Development测试工程及独立GUID档池；没有Shipping打包、桌面导入、提交、推送或公开发布。分支codex/TASK-053-title-wheel，真实基线4db5789184fe38e041d62a1e68c8517338ea0b01，任务Active供Owner分析。上一版背景改造的完整历史记录见[skills-preview-v1/REPORT.md](../skills-preview-v1/REPORT.md)，旧PASS不替代本轮源码验证。

四分支的29个原节点和25条前置连线仍来自原内容表，100%—150%字号均保持树状加点。右侧显示原技能说明、真实等级、前置和成本；原Learn／ResetSkills、点数预算、等级上限、效果及存档规则未修改。背包三栏／四分类、HUD厚原色条、弟弟位置、约五秒任务通知和十米指引过滤继续保留。

双击[主界面UI测试.cmd](../../../../scripts/ui/主界面UI测试.cmd)，自然地图加载结束后按K查看技能，Tab查看背包，Esc返回。使用已有UE5.8.2／Python／GameFactory环境，每次启用独立GUID测试池。桌面“归火”仍运行0.2.0-preview.20261002.2。实际游戏截图：[默认技能页](../hud_caa4d34d66ae/skills-default.png)、[感知分支](../hud_caa4d34d66ae/skills-sense.png)、[学习后](../hud_caa4d34d66ae/skills-learned.png)；原生截图：[150%字号](../verify_314d25bd287f/verify_314d25bd287f-skills-text-150.png)、[720p](../verify_314d25bd287f/verify_314d25bd287f-skills-720p.png)、[16:10](../verify_314d25bd287f/verify_314d25bd287f-skills-16x10.png)、[超宽](../verify_314d25bd287f/verify_314d25bd287f-skills-ultrawide.png)。

## 当前验证

- [skills_refine_build_r1](../skills_refine_build_r1/build_result.json)：UEClient公开API构建HearthwardEditor／Win64 Development成功，错误与诊断为空。
- [verify_314d25bd287f](../verify_314d25bd287f/report.json)：409/409界面检查、63张原生UI截图；技能100项覆盖标题／导航移除、旧导航区域无点击目标、双螺旋元素、全部节点命中与原说明、前置阻挡、原点数／学习／等级上限／洗点、连线及150%字号、720p、16:10、超宽显示。原标题、设置、存档、确认框高亮26项、HUD51项、背包80项及实际PIE延时装备检查仍通过。夹具还原Gameplay、库存与设置并确认保存节点数不变。独立技能记录见[skills-preview.json](../verify_314d25bd287f/skills-preview.json)。
- [hud_caa4d34d66ae](../hud_caa4d34d66ae/report.json)：真实自然新游戏44/44检查、13张GameViewport截图，覆盖默认／感知／学习后技能页、原根与子技能学习、点数扣除及免费洗点返还。旧任务通知约五秒后消失，日志仍完整，返回HUD不重播；背包四分类仍可用。实际三维十米边界11/11，见[hint-range.json](../hud_caa4d34d66ae/hint-range.json)。学习事件回执依照原规则保留在可弃测试池。
- [hud_3e8bfdbfe228](../hud_3e8bfdbfe228/report.json)：实际Development游戏入口自然加载结束后连续三次窗口响应通过。
- 三轮各28项配置／UI源码／DLL／原技能内容与成长规则指纹均匹配当前文件，验证自有进程均已停止，启动器恢复开发用户设置。绑定见[source-binding.json](source-binding.json)。
- 七张原生技能PNG完全不透明（alpha255）；默认、150%字号、720p、16:10、超宽和三张自然地图技能画面已视觉检查。左上标题和导航移除清晰可见，双螺旋保持低对比，不盖过说明。见[opacity-review.json](opacity-review.json)与[visual-review.json](visual-review.json)。原背包组合、Gameplay／Progression及技能内容表保持原规则。
- 仓库、33/33工具测试、差异和本地授权范围检查的实际结果见[validation.json](validation.json)与[scope-audit.json](scope-audit.json)。正式基线批准快照检查仍单独记录失败：真实基线没有未提交的TASK-053批准快照，本地审计不替代该检查。
- 八项桌面EXE／UI、正式玩家档和用户设置与修改前SHA256一致；BUILD-INFO也保持一致。见[preservation-result.json](preservation-result.json)和[installed-version-preserved.json](installed-version-preserved.json)。

## 历史失败与验证边界

上一版的命名遮蔽构建错误、Python未反射调用和首轮启动未在120秒进入自然地图均保留在[历史报告](../skills-preview-v1/REPORT.md)。当时相同参数的新进程通过，首轮启动原因未确认；本轮未修改启动逻辑，新启动验证通过。本轮构建和三次运行均通过。

原生事件检查不等于Windows物理键鼠完整实玩，Owner视觉验收尚待进行。当前UI未做Shipping构建或桌面导入。前轮HUD／背包报告只绑定当轮源码；后续源码改变后需要新的构建与验证。
