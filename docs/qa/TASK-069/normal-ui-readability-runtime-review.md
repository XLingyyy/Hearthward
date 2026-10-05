# TASK-069 正常流程界面可读性定向复测

2026-10-05，Root 串行公共 UEClient，UE 5.8.2 Development Editor `-game`，Bootstrap 正常入口，同一独立池 c5eb9b97-b99d-41e9-8a1f-ce19708dc24e。实际 OS 键鼠；没有 Python 世界夹具、物资授予、位置设置或控制台玩法命令。受测为当前未提交补丁，基线 67fb0784ca8c6d488173e587e7f95c4be0d9092a。

## 已确认结果

| 局部问题 | 修改与实际结果 | 原始截图 |
|---|---|---|
| 标题、暂停和背包装饰文字增行／页脚越底 | 13 个已有装饰文本沿原 fontRole 字段恢复原字号；功能文字保持原正文下限。100% 显示没有旧页脚增行 | normal-title-os-green.jpg、normal-pause-footer-os-green.jpg |
| 默认背包键帽重叠、装备标签换行、亮地面低对比 | 现有五对键帽与标签分隔；装备标签保留原中心并加宽；原 panel 色用于四块已有区域及标签局部底色，中央角色仍可见 | normal-inventory-os-green.jpg |
| 最大字号背包底色和键名缺失 | generic 使用同一正文字体测量／绘制；重排使用现 panel 深底；标签沿当前实际 keycap 文本组合，五个键名完整保留 | normal-inventory-150-top-os.jpg、normal-inventory-150-hotkeys-os.jpg |
| Save 政策与安全提示重叠 | 政策、安全提示及节点列表分行，两个正常节点可见且没有交叠 | normal-save-os-green.jpg |
| Settings 大字行当前值消失 | 仅 ReadableLayout 的 settingsRow 组合真实 Label 与 Value；150% 实际显示“界面文字 · 150%”，正常点击两次减小与应用后恢复100% | normal-settings-150-value-green-os.jpg、normal-settings-100-restored-os.jpg |
| Pause 长目标与时间交叠 | 目标显式 Font24，332宽／236高；时间和位置改为组内同行，生存动作位置保持。直接冷重启、继续、实际 Escape 后，夜袭三段目标全文显示，时间／位置分离 | normal-pause-objective-green-os.jpg |

默认修复首轮实际 client1920×1080（native1922×1128）。第三进程实际应用150%后显示模式变为无边框2560×1600，背包最大字号截图绑定该尺寸；不计为150%@1920、4K或全部Windows DPI通过。第四进程重新窗口启动后，150%设置当前值截图绑定client1920×1080；应用100%时引擎再次使用无边框2560×1600。第五进程窗口100%直接继续，Pause截图绑定client1920×1080。

## 进程与输入边界

第三进程 PID38792、run ca16424f-a39b-49d2-b7d1-f8ba447f2566 为600秒TIMEOUT，公共 stop_ok，未在该进程恢复100%或正常Quit。第四进程 PID35788、run9aa8732c-3d9c-4afe-8db0-ecfe7c26a672 已实际Apply恢复100%，host0／stop_ok。其后 Continue HUD正常tick，OS Escape／P／Tab未打开菜单；没有把这次host0计为正常退出，现已在原Apply路径复现并修复，后续相同OS路径通过；见[加载输入报告](loading-page-input-runtime-review.md)。

第五进程 PID46804、run67d700e4-4048-46ea-a1e5-12683138c33b：窗口100%直接Continue，实际Escape进入Pause，鼠标返回标题确认及Quit确认，日志记录 RequestExitWithStatus(0,0,UGameEngine::HandleExitCommand)，随后公共清理stop_ok。前一正常第二进程已独立确认自然Quit窗口消失及Preparing to exit；见 normal-game-os-runtime-review.md。

## 全部任务目标的定向量字

现有 ExperienceTests 单项读取实际 Widget 的字体、tracking、fontRole、全文和几何，按生产 Paint 相同算法用 Slate FontMeasure 测量。全部34个目标在正常和实际受伤倒地两种状态通过；最多六行，全文容纳且不碰时间／位置及实际生存按钮，放弃按钮真实命中目标保持。1/1 PASS，0错误、1既有 EnhancedInput LocalPlayer 初始化警告；这项夹具不提供正常 OS 倒地信用。见 pause-all-objectives-native-green.json。

## 尚未取得的信用

这批截图只验证表中局部问题。全部页面／状态、所有规定分辨率、显示确认恢复完整矩阵、Right Ctrl/Shift实际OS、声音、正式玩法全篇、Owner体验和Shipping均未由这些截图验收。进一步复核确认Inventory分类原模板文字非空，Element已把实际字号夹到24，无需修改；Map探索原模板为空，100%实际17，125/150已沿现有重排夹到30/36。默认Map字号17→24的实际Widget测试1/1通过，全文145×28、bottom222/barY223，见[地图探索字号报告](map-exploration-font-runtime-review.md)；Pause正文保持24。

原始 host JSON 与截图保留在本目录；各run唯一输出在 Saved/Task069。无模型bundle部署与AI提交；-NoSound，声音未测试。
