# TASK-053 本地桌面 UI 导入

2026-10-02，Owner确认保留最新登录、设置和存档UI，并授权导入本机桌面游戏。当前版本 `0.2.0-preview.20261002.1` 已替换桌面原目录，原GPU／CPU启动入口继续使用，新建桌面“归火”快捷方式。此交付仅更新本机游戏；源码未提交、推送或合并，GitHub Release未更新。

## 构建与版本绑定

基线 `4db5789184fe38e041d62a1e68c8517338ea0b01`，分支 `codex/TASK-053-title-wheel`，包含当前未提交的TASK-053实现。UE 5.8.2、Win64 Shipping，通过GameFactory UEClient公开 `build.package` 执行BuildCookRun；Editor Development与Shipping目标重新编译，地图Bootstrap／Natural Wilds烘焙、Stage及Archive全部成功，耗时186.46秒。命令与真实返回见[package-result.json](package-result.json)，完整输出见[package.log](package.log)。

[source-manifest.json](source-manifest.json)绑定292个Source／Config／Resources／插件源码文件；清单SHA256 `d2456f4833f1c55908cf4f844cc9e6716864c136533350adb8f142a83c567936`，构建后所有文件仍匹配，见[source-match.json](source-match.json)。Shipping可执行文件SHA256 `f499664761d20ace2eab011b4546b06095d13d70f68703b0d6910dfa35aff610`。记录见[build-info.json](build-info.json)。两个包内UI JSON与批准源码一致，六个logo UV仍复用原PNG。

首次构建[desktop_20261002_ui](../desktop_20261002_ui/package-result.json)失败：CampaignWorld和WorldPresentation调用了只存在于WITH_EDITORONLY_DATA的ADirectionalLight::GetComponent。两处仅改为FindComponentByClass<UDirectionalLightComponent>()，灯光数值与其他逻辑未改；随后重建成功。版本标识与Inno脚本AppVersion同步递增，但本次未编译Inno安装器。

## 安装、存档和启动

实际目标：`C:/Users/22543/Desktop/Hearthward-20260929-9058ee2/Windows`。新包先复制到临时候选目录，143个文件逐个SHA256一致后才切换；完整旧Windows目录移动到 `备份-TASK053-20261002/Windows`，未执行递归删除。部署脚本对所有移动目标检查绝对路径和重解析点，玩家Saved目录不进入包。首次部署被Saved检测拒绝，因为生成输出的父目录也名为Saved；已修正为只检测包内相对路径，原程序在该拒绝时未改变。最终记录见[deploy-result.json](deploy-result.json)。

模型大小及SHA256符合config/local-ai.lock.json，两套CPU／Vulkan运行库在包内，见[bundle-check.json](bundle-check.json)。旧139个程序文件完整保留，旧Shipping文件指纹与原BUILD-INFO一致，新Shipping指纹与新构建一致，见[backup-check.json](backup-check.json)。

玩家AppData下3个存档文件与原用户设置在更新、启动核对后均保持原字节指纹；副本保存在同一桌面备份目录的PlayerSaved。玩家档内容未进入本任务证据或版本库；核对结论见[player-preservation.json](player-preservation.json)。

通过UEClient启动安装后的真实Shipping文件，使用原GPU入口相同的Vulkan／16层参数。最终进程PID14472，窗口标题Hearthward，窗口句柄有效且Responding=true，见[installed-launch-held.json](installed-launch-held.json)及[installed-process.json](installed-process.json)。为保持这个由命令行启动的交互窗口，启动宿主持续存活至玩家关闭游戏；正常双击桌面快捷方式不依赖该验证宿主。窗口曾留给Owner游玩；收尾检查时进程已关闭，未调用停止此最终游戏的工具，关闭原因未观察到。启动成功结论绑定之前的窗口响应快照，后续仍可从桌面快捷方式启动，见[window-end-status.json](window-end-status.json)。

早期一次性启动记录[installed-launch.json](installed-launch.json)没有保留交互进程，不能用作最终启动成功证据。[shipping-smoke](shipping-smoke/summary.json)也未生成截图／原生录制报告，不能视为Shipping UI画面验证。未恢复Windows物理输入自动操作。

## 当前 UI 验证及边界

版本号与运行时访问器修正后重新执行[verify_b5b51ab92d9e](../verify_b5b51ab92d9e/report.json)，139/139项真实PIE检查、32张引擎UI截图通过，覆盖三页、四项菜单与边界、分页、键位入口、锁定／确认／取消、返回及150%字号。19个当前UI源码／配置／Editor DLL指纹一致，见[scope-audit.json](../scope-audit.json)。这些是Editor／Widget命令验证，不能表述为Shipping物理键鼠或完整玩法验收。

33项工具测试通过，见[tool-test-summary.json](tool-test-summary.json)。本地批准范围检查通过，正式基线任务快照检查仍因053尚未在基线批准提交中存在而不可通过，未绕过该检查或自行提交。

当前已验证Shipping打包、安装文件、模型、原程序备份、玩家档保留及真实窗口启动。Shipping三页自动截图、完整物理滚轮／鼠标／键盘路线、完整游戏流程、两台机器和安装器测试为NOT_RUN。
