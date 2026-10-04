# TASK-053 全部最新UI同步本地桌面（2026-10-03）

本机桌面版已从 `0.2.0-preview.20261002.2` 更新为 **0.2.0-preview.20261003.1 / Win64 Shipping**，当前全部UI替换旧版：登录、设置、存档、HUD、背包、技能、日志及各项最新修复。双击桌面“归火”，或安装根目录原GPU／CPU启动cmd即可进入新版。

用户本次明确要求“联系上下文，将当前所有已更新的UI同步到本地桌面端，替换旧版UI”，已在[TASK-053](../../../tasks/TASK-053.json)记录，替代此前后四页仅测试、不部署的限定。当前分支 `codex/TASK-053-title-wheel`，基线 `4db5789184fe38e041d62a1e68c8517338ea0b01`；本次只部署本机安装，源码未提交／推送／合并，未公开发行。

## 已同步内容

- 登录：纯黑视口、约70%错位归火和居中火苗，四项滚轮菜单、首尾箭头；退出确认返回按钮无悬停时不反复高亮。
- 设置／存档：宽幅不透明炭灰黑金面板、分类与分栏、长列表分页及100%—150%字号；原设置应用、显示确认、键位、保存、载入、锁定、删除和返回语义保留。
- HUD：删除指定教程文字／提示框；厚重红黄绿生命／饱食／体力条移到左上，弟弟在条下，任务在弟弟下只于新收到后约五秒显示。发现指引限定目标十米三维范围；四道具选中居下高亮，其余分列左右上方并保持可辨识。
- 背包：不透明厚边三栏，仅装备、材料、食物、工具四项；任务道具与图纸归入工具，原物品／角色真实属性和操作保留。
- 技能：左上技能标题、取消旧导航／归火标志、右下淡反向双螺旋；四树29节点25边等距对齐、单分支垂直、一分二父节点居中，保持原前置及加点／洗点。
- 日志：六原符号横排顶栏，左列表右详情；未解锁行只保留原位与分隔线并留白。仅选中分类及下划线高亮，六图标均为暖白金色且有柔和光晕，未选中均为灰色；原追踪／地图／领奖／历史规则保留。

## 当前源码与构建证据

1. [Development构建](../desktop_all_ui_build_20261003/build_result.json)：UEClient公开API执行HearthwardEditor / Win64 / Development，成功，错误和诊断均为空。本轮仅递增本地预览版本，没有再改已验证的UI呈现代码。
2. [原生界面回归](../verify_15122320c6bf/report.json)：**518/518**，77张未编辑原生PNG。覆盖菜单／退出26项焦点回归、设置与存档、HUD51项、背包80项及实际延时卸装／重装、技能100项、日志108项及六分类切换／连续刷新／大字号／留白。
3. [自然地图回归](../hud_3b716bdceb34/report.json)：**56/56**，19张实际GameViewport PNG，覆盖五秒通知、返回HUD不重播、四道具和四背包分类、技能原学习及洗点、六日志分类；[三维十米边界](../hud_3b716bdceb34/hint-range.json) **11/11**。
4. [日志亮度像素审计](highlight-pixels.json)：12张原生及自然地图PNG通过，只有当前选中符号为暖金色，选中峰值至少215/255且达到主线参考90%；同一审计检测到历史主线常亮和历史支线偏暗缺陷。[Development绑定](development-validation.json)记录两轮各29项源码／配置／DLL／规则指纹和测试脚本指纹；两轮自有Editor进程停止、开发设置恢复。
5. [Shipping构建](package-result.json)：UEClient公开API执行BuildCookRun，对引导页及自然地图进行Cook／Stage／Archive，**PASS**，用时156.15秒；[完整日志](package.log)。[源清单](source-manifest.json)297文件绑定当前Source／Config／Resources／插件源码，[清单SHA256](build-info.json) `8ad3cbf24ad6f56d98d003fd84a40fa3db5ce629ecb71e9428154494cf0970a6`。
6. [包核对](archive-validation.json)：297源码仍匹配、45资源逐一与源码一致、开发回归指纹仍当前、模型大小和指纹与锁文件一致、包内无开发Saved目录。Shipping执行文件SHA256为 `5e39dfabfebf804ec35362efe4e70687cacc5078e747a4b14bbd5c58167a8cfd`。

## 安装、备份与启动

安装位置：`C:/Users/22543/Desktop/Hearthward-20260929-9058ee2/Windows`。先复制候选目录并核对全部143文件，再将原Windows整体移动到已检查的安装内备份路径，最后候选换入；没有递归删除。[部署记录](deploy-result.json)、[独立核对](deployment-validation.json)保存每文件SHA256。

完整旧版位置：`C:/Users/22543/Desktop/Hearthward-20260929-9058ee2/备份-TASK053-20261003_all_ui/Windows`，143文件与[部署前清单](../desktop_sync_20261003_preflight/before.json)完全一致。PlayerSaved备份中15个SaveGames／Config文件与原始字节一致，旧安装根BUILD-INFO和界面说明分别保存；此前两份备份保留。原桌面快捷方式和CPU／GPUcmd未改变，安装根与Windows内BUILD-INFO相同；[新版界面说明记录](desktop-notes-update.json)包含旧说明的备份指纹。

[Shipping实际启动](shipping-startup-all_ui.json)：公开API直接启动新安装Hearthward-Win64-Shipping.exe，沿用Vulkan／16层参数。自有PID30984连续六次观察，最后三次窗口存在、标题正常且响应，32.13秒后按公开API停止自有进程。部署期间存档／Config无变化，启动前后三个存档文件与GameUserSettings.ini逐字节指纹相同。

## 仓库检查与验证边界

工具测试 **33/33**、仓库自检 **0错误**、git diff --check和[当前用户授权范围审计](scope-audit.json)通过；[原始命令结果](repository-checks.json)分别记录。正式基线快照检查仍为FAIL，唯一错误为TASK-053在基线缺少可用获批任务快照，本地授权范围审计不替代。首轮文档检查发现归档README／交接原文的相对链接被当作新页面校验，现以.md.original保存原始字节，并补齐检查证据链接后复验通过；[首轮原始结果](repository-checks-initial.json)保留。

界面回归使用原生Widget事件和Development真实渲染，Shipping已验证构建、资源一致性及实际程序窗口启动；本轮没有Shipping逐页物理键鼠操作或Shipping界面截图，也不是完整玩法、性能或Owner视觉验收。程序及资源替换已完成，任务Active等待Owner继续分析。历史.20261002.2安装及此前各UI测试记录只绑定其当轮源码。
