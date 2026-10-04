> 历史版本：已修复只有选中分类高亮，但其他分类的选中图标尚未提高到主线参考的亮度。当前证据见../journal-preview/REPORT.md。

# TASK-053 日志分类高亮修复测试版（2026-10-03）

日志已改为顶部六个原分类符号、左侧纵向条目、右侧对应详情的两栏结构。六个符号依次为主线任务、支线任务、世界见闻、人物档案、势力阵营、收集要素，顶部只呈现符号；当前分类名称位于左侧列表标题。只有当前选中的分类图标和下划线呈金色；主线图标取消选中后恢复中性灰色，悬停其他图标不会出现第二处高亮。页面使用完整不透明的炭灰底、细金线、金色选中光带及原字体，与当前设置、背包及技能测试页一致；取消旧场景插画、三栏及总导航。

左侧默认显示8行，130%—150%字号显示6行，行距与条目副信息统一。右侧任务按标题、地点、目标／真实进度、详情／原经验显示、同行说明及操作分区；其他分类展示原名称、描述、原图标和收录／实际持有量。未解锁行保留原行位、行高和分隔线，名称、状态、菱形标记及选中光带全部留白，不再显示未解锁任务／未知条目／未收录等占位描述。没有已知条目的分类，右侧详情同样留空。空行不可点击，上下键跳过空行选择已知条目；不压缩或重排原条目。收集历史依旧按collected事件解锁，库存归零不抹去记录。原Campaign任务过滤、QuestAvailable／Progress／Track／Claim、地图地点与存档规则未修改；日志记录页阻止快捷键误操作此前选中的任务。

双击[主界面UI测试.cmd](../../../../scripts/ui/主界面UI测试.cmd)，自然新游戏加载完成后按 **J** 打开日志。依赖本机已准备的UE5.8.2／Python／GameFactory环境，每次使用独立GUID测试池。点击符号或Q／E切换分类；点击条目或上下键选择，滚轮、PageUp／PageDown、Home／End及页底按钮浏览长列表；F地图、V追踪，Esc返回。任务领奖、地图定位、原设置入口与返回关系保留。

仅更新同一Development测试工程，未进行Shipping构建、桌面导入、提交、推送、合并或公开发布。桌面安装版仍为0.2.0-preview.20261002.2。之前的HUD、背包、规整技能树和双螺旋界面保留。分支codex/TASK-053-title-wheel，基线4db5789184fe38e041d62a1e68c8517338ea0b01，任务Active等待Owner分析。

## 真实画面

[支线选中、主线灰态的原生截图](../preview-journal-highlight.png)展示此次修复。已知内容来自独立测试夹具，正式玩家进度未修改。

实际自然地图六分类：[主线](../hud_00b69586e966/journal-main.png)、[支线](../hud_00b69586e966/journal-side.png)、[世界](../hud_00b69586e966/journal-world.png)、[人物](../hud_00b69586e966/journal-people.png)、[阵营](../hud_00b69586e966/journal-factions.png)、[收集](../hud_00b69586e966/journal-collection.png)。未到达条件的分类保留分隔线，未知行及没有已知内容的详情区域均留白；截图没有人工填入解锁状态。

独立原生夹具的已知收集示例：[物品记录](../verify_0550668a7af1/verify_0550668a7af1-journal-collection-known.png)。适配截图：[150%字号](../verify_0550668a7af1/verify_0550668a7af1-journal-text-150.png)、[720p](../verify_0550668a7af1/verify_0550668a7af1-journal-720p.png)、[16:10](../verify_0550668a7af1/verify_0550668a7af1-journal-16x10.png)、[超宽](../verify_0550668a7af1/verify_0550668a7af1-journal-ultrawide.png)。均为引擎原始PNG；没有图片编辑或复用参考游戏素材。

## 当前验证

- [journal_highlight_build_r2](../journal_highlight_build_r2/build_result.json)：通过GameFactory UEClient公开API构建HearthwardEditor／Win64 Development成功，错误与诊断为空。
- [verify_0550668a7af1](../verify_0550668a7af1/report.json)：518/518界面检查及77张原生PNG通过。日志108项、14张PNG，新增六分类分别在悬停其他分类并连续30次刷新后仅选中图标／下划线高亮，强化Q／E切换断言；继续覆盖六符号原生点击、未知行无占位文字／标记／光带且不可点击、空详情无提示、上下选择跨过未解锁空行、两栏坐标、Q／E、上下选择、滚轮与首尾／翻页、模态输入接管、原任务追踪、未完成与重复领奖阻挡、原奖励和后继解锁、地图／设置返回、150%字号及分辨率适配；夹具还原Gameplay、库存、经验、回执和设置，保存节点数量未变。详见[journal-preview.json](../verify_0550668a7af1/journal-preview.json)。原菜单、退出高亮26项、HUD51项、背包80项、技能100项及实际PIE延时卸下／重装继续通过。
- [hud_00b69586e966](../hud_00b69586e966/report.json)：真实自然新游戏56/56检查与19张GameViewport截图，包含六类日志和原任务全文；五秒通知消失与回到HUD不重播、原四类背包、规整技能树学习及洗点通过。真实三维十米边界11/11见[hint-range.json](../hud_00b69586e966/hint-range.json)。
- [hud_ec37b5106ad1](../hud_ec37b5106ad1/report.json)：实际Development测试入口自动新游戏，自然地图加载结束后连续三次窗口响应通过。
- 三轮各29项配置、UI源码、DLL、原数据与Gameplay／成长规则指纹匹配，6项验证脚本指纹记录见[source-binding.json](source-binding.json)。三轮启动器均停止自有进程并恢复开发设置。
- 原生六类已知状态及自然地图六类共12张原始PNG，像素审计确认每张只出现当前选中分类的金色图标，并且它比所有未选中图标更亮；旧版支线截图能复现并被该审计检测出缺陷，见[highlight-pixels.json](highlight-pixels.json)。
- 原生14张日志PNG全像素alpha为255，见[opacity-review.json](opacity-review.json)。人工查看六类自然画面、六类原生已知内容及150%字号，选中图标／下划线唯一高亮，未解锁留白和原描述保持，见[visual-review.json](visual-review.json)。
- 八项桌面程序／UI／正式玩家档／用户设置与安装BUILD-INFO逐字节指纹保持不变，见[preserved-after.json](preserved-after.json)。
- 工具测试33/33、仓库自检、差异检查和本地授权范围审计，实际记录见[validation.json](validation.json)、[repository-checks.json](repository-checks.json)及[scope-audit.json](scope-audit.json)。

正式基线任务范围检查仍为FAIL：基线没有未提交TASK-053批准快照（No usable approved task snapshot at base: TASK-053）。本地审计通过用户已批准路径、其他页面配置、范围外组合、原仓储／背包与Gameplay规则及当轮指纹，不替代该正式检查。当前没有Git提交授权。

本轮改动限于日志分类图标着色、顶部分类选择高亮和相关原生回归；原先102项检查保留，新增6项到108项，增量范围见[highlight-change-scope.json](highlight-change-scope.json)。原因是旧主线素材自带金色，而通用图片绘制忽略元素颜色；现仅对六个日志顶部图标应用颜色，并让顶部下划线只由选中状态决定。原素材与其他页面绘制不变。首轮journal_highlight_build_r1及verify_a466743e68fd／hud_9547e421b0b7功能验证通过，但灰态主线仍偏亮，视觉核对后调整灰态和选中亮度；最终证据以r2三轮为准。修改前源码与文档保存在highlight-before，上一版留白结果见[历史留白报告](../journal-preview-v2/REPORT.md)，原日志布局及其首轮编译修正见[初版日志报告](../journal-preview-v1/REPORT.md)。旧PASS不作为当前版本验证。

验证使用原生Widget事件、原命令和真实引擎画面；尚未完成Windows物理键鼠实玩及Owner视觉验收。本轮未编译当前UI的Shipping版本；后续桌面导入需用户另行授权。实际成长／领奖只在可弃测试池执行。
