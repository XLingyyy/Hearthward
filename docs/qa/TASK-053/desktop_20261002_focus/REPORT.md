# 退出确认框高亮修复版桌面导入

2026-10-02，用户要求修复鼠标未悬停时返回按钮反复亮起，延续最新登录、设置和存档UI的桌面导入授权。当前本机桌面版本 `0.2.0-preview.20261002.2` 已安装，双击原“归火”快捷方式或GPU／CPU启动cmd即可运行。

## 构建与源码绑定

UE 5.8.2、Win64 Shipping，经GameFactory UEClient.build.package公开API执行BuildCookRun；编译、Bootstrap／Natural Wilds烘焙、Stage与Archive成功，用时203.75秒。基线4db5789184fe38e041d62a1e68c8517338ea0b01，加codex/TASK-053-title-wheel本地未提交修改。

[构建结果](package-result.json)和[完整输出](package.log)保留实际命令及返回。[源码清单](source-manifest.json)绑定293项Source／Config／Resources／插件源文件，清单SHA256 `9f8149a46e18058bab5607fb31f793872ddba807332332f49d19b9fbcf685065`；当前全部匹配，同时20项UI源码／Editor DLL／配置验证指纹仍匹配，见[最终绑定](current-source-verification.json)。Shipping程序SHA256 `c6e329d8bfda29fed7498e73bc024684a2a9d82306d446fd4c690654921b5d70`；两份UI JSON与批准源码一致，见[构建信息](build-info.json)。

## 安装与数据保留

安装目录 `C:/Users/22543/Desktop/Hearthward-20260929-9058ee2/Windows`。候选包143文件逐个SHA256一致后换入；上版0.2.0-preview.20261002.1整体移至 `备份-TASK053-20261002_focus/Windows`，无递归删除，原版备份 `备份-TASK053-20261002/Windows`继续保留。实际部署复用既有且目标／参数匹配的“归火”快捷方式，未覆盖未知快捷方式。见[部署记录](deploy-result.json)及[上版备份核验](backup-check.json)。

更新前AppData下存档和配置完整快照复制到本轮桌面备份的PlayerSaved，复制与部署期间指纹一致。Shipping启动前后3个存档文件和GameUserSettings.ini原字节指纹保持一致，未部署开发者Saved或临时测试档。Qwen3.5-4B Q4_K_M的大小2740937888及SHA256符合锁文件，见[模型核验](bundle-check.json)。

## UI及启动验证

`verify_fb927cae9e15` 166/166检查、37张引擎UI截图通过，包含26项高亮回归：12组状态各30次NativeTick周期刷新、真实Native鼠标／键盘事件及安全取消。见[总体报告](../verify_fb927cae9e15/report.json)和[修复说明](../modal-focus.md)。

安装后的实际Shipping进程PID19444通过UEClient启动，沿用GPU入口Vulkan／16层参数。3次窗口观察均有有效句柄、标题Hearthward且Responding=true，跨15.91秒；观察完成仅停止本次自有进程，未留下测试窗口。完整记录见[最终启动检查](shipping-startup-r3.json)。

首轮[启动记录](shipping-startup.json)实际6次窗口观察均正常，自动判定错误：UE标题带尾空格，而检测要求完全相等；同时把每次启动新增的CrashReportClient诊断ini误算为玩家设置变化。原始失败报告保留；检测已对窗口标题去尾空格，只比较真实存档及GameUserSettings.ini。第二轮观察会话在查询进程时返回非零，未产生完整报告，退出原因未观察，见[错误记录](shipping-startup-r2-error.json)；不计PASS。第三轮最终启动和用户数据检查通过。

## 边界

高亮测试经原生Widget事件与真实引擎绘制完成，未注入物理Windows键鼠；Shipping验证构建、文件及窗口启动，未声称自动操作Shipping弹框或覆盖完整玩法。工具测试33/33，仓库结构及本地范围审计通过。正式基线快照仍缺少未提交TASK-053，不绕过检查；未提交、推送、合并或公开发行，Inno安装器未构建。
