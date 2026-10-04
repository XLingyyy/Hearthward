> 首版历史记录，仅绑定该轮源码。最新标题与反向双螺旋修改见[当前报告](../skills-preview/REPORT.md)。

# TASK-053 技能界面测试版（2026-10-03）

用户要求保留原树状加点形式，背景参考所附第二幅图。当前沿用同一Development测试工程与独立GUID存档池；本轮没有Shipping打包、桌面部署、提交、推送或公开发布。分支codex/TASK-053-title-wheel，真实基线4db5789184fe38e041d62a1e68c8517338ea0b01，任务Active供Owner分析。

技能页改为完整不透明的炭灰雾面背景，带低对比雾状明暗与淡环形刻纹。四个分支采用轻薄分隔面板，移除原页面的大幅场景图、皮革表面与动物装饰，复用原归火标志、分支和节点图像，延续黑金文字与柔和选中光带。四树展开占用原左侧场景空间，右侧集中展示原技能说明、真实等级、前置和技能点成本，底部保留返回、学习键位与免费洗点。

全部29个原技能节点及25条前置连线来自原内容表；节点点击、F学习、原Learn／ResetSkills、技能预算、效果和存档规则不变。等级增益以原说明为准，未引入参考游戏的能力或数值。100%—150%字号始终保留树状布局，节点和连线不被原大字号文字列表替换。描述与等级、前置、学习操作分区排布。

双击[主界面UI测试.cmd](../../../../scripts/ui/主界面UI测试.cmd)，自然地图加载结束后按K查看技能，Tab查看背包，Esc返回。使用已有UE5.8.2／Python／GameFactory环境，每次使用独立GUID测试档池。桌面“归火”仍运行已安装0.2.0-preview.20261002.2。实际游戏截图：[默认技能页](../hud_6e4341969c3b/skills-default.png)、[感知分支](../hud_6e4341969c3b/skills-sense.png)、[学习后](../hud_6e4341969c3b/skills-learned.png)；原生截图：[150%字号](../verify_36cda9dd9514/verify_36cda9dd9514-skills-text-150.png)、[720p](../verify_36cda9dd9514/verify_36cda9dd9514-skills-720p.png)、[16:10](../verify_36cda9dd9514/verify_36cda9dd9514-skills-16x10.png)、[超宽](../verify_36cda9dd9514/verify_36cda9dd9514-skills-ultrawide.png)。

## 当前验证

- [skills_build_r2](../skills_build_r2/build_result.json)：UEClient公开API构建HearthwardEditor／Win64 Development成功，错误和诊断均为空。
- [verify_36cda9dd9514](../verify_36cda9dd9514/report.json)：405/405界面检查、63张原生UI截图。新增技能96项覆盖全部节点命中与原说明、前置阻挡、真实点数、F事件学习、等级上限、免费洗点、分支进度、连线更新、不同字号／视口和返回；原生夹具还原玩法、库存和设置并确认存档节点数不变。原标题、设置、存档、高亮修复26项、HUD51项、背包80项及实际PIE延时装备检查继续通过。技能独立记录见[skills-preview.json](../verify_36cda9dd9514/skills-preview.json)。
- [hud_6e4341969c3b](../hud_6e4341969c3b/report.json)：真实自然新游戏44/44检查、13张GameViewport场景截图，含默认／感知／学习后技能页。原技能点扣除、前置学习及免费洗点返还通过；学习事件回执按原规则保留在该轮可弃测试池。旧任务通知仍约五秒后消失且日志完整，背包四分类保留。实际三维十米边界11/11，见[hint-range.json](../hud_6e4341969c3b/hint-range.json)。
- [hud_78d42a519358](../hud_78d42a519358/report.json)：实际Development独立游戏启动自然地图，loading end后连续三次窗口响应通过。
- 上述三轮各28项配置、UI源码、DLL、原技能内容与成长规则指纹全部与当前文件一致，自有进程均已停止，启动器恢复开发用户设置。见[source-binding.json](source-binding.json)。
- 七张不同视口／状态的原生技能PNG均完全不透明（alpha255），见[opacity-review.json](opacity-review.json)。默认、放大字号、720p、16:10、超宽及三张自然地图技能画面已视觉抽查，见[visual-review.json](visual-review.json)。原背包三栏组合未改，原Gameplay／Progression及内容表与真实基线相同。
- 仓库自检、33/33工具测试、差异检查和本地授权范围审计记录见[validation.json](validation.json)及[scope-audit.json](scope-audit.json)。正式基线批准快照检查仍失败：真实基线没有未提交的TASK-053批准快照；未将它标成PASS，本地审计不替代正式检查。
- 八项桌面EXE、UI、正式玩家档及用户设置与修改前SHA256一致；安装BUILD-INFO也一致，见[preservation-result.json](preservation-result.json)和[installed-version-preserved.json](installed-version-preserved.json)。

## 原始失败与限制

skills_build_r1因局部Navigation名称遮蔽UWidget成员而失败，已改名并重新构建。hud_2aa454ab49bb的截图脚本调用未反射的SaveSnapshot方法失败，已改为原有学习／洗点动作并重跑；未扩大Gameplay公开接口。hud_b1885f853e1e首轮独立窗口未在120秒内进入自然地图，保持失败记录；相同参数的新进程hud_78d42a519358正常完成启动。本轮没有修改启动逻辑，未声称已经确认首轮未进入地图的原因。

原生事件检查不等于Windows物理键鼠完整实玩。Owner视觉验收、当前测试UI的Shipping构建与桌面导入尚未进行；后续部署需用户另行授权。此前[背包报告](../inventory-preview/REPORT.md)和HUD报告只绑定各自历史源码，旧PASS不替代本轮验证。任何后续源码变化都需要新的构建及验证绑定。
