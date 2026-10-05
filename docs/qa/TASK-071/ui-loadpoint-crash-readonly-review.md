# TASK071 UI 首跑 crash 与最窄诊断

实际首跑状态 CRASH，不能计为 UI 业务 PASS。独立 pool `cdcb4a0a-fa54-4984-9f59-ce0eee1c30e9`、PID31404；root报告 host 到360秒超时，进程已结束。已存在 launch/stop，cases/boundaries 为空，无 results.json，原脚本没有中间 stage/check 文件。

## 已确认事实

匹配 PID 的原生 crash archive 为 `Saved/Crashes/UECC-Windows-28D077924965C5033CABD2A24B1B0132_0000`，SecondsSinceStart45，IsAssert/IsEnsure 均 false，异常为读地址0x3d的 EXCEPTION_ACCESS_VIOLATION。

Slate.txt 明确 Game thread、Prepass，最终路径包括 `SObjectWidget: SScaleBox: SBox`；SBox 的来源为 UMG SizeBox.cpp35。当前日志13:24:35 frame424的未知栈落在SlateCore/Slate，没有可定位的Hearthward函数。崩溃前最后frame423在13:24:31.648–31.804连续出现三组 viewport input mode切换：LockOnCapture→DoNotLock、DoNotLock→LockOnCapture、LockOnCapture→DoNotLock。

公开证据保存于同目录 `ui-crash-31404-evidence.json` 与 `ui-crash-31404-Slate.txt`。未复制/打印整个 CrashContext（包含其它主机资料），未重复栈，无checksum。

## 事实的限制

原 verifier 在一个 Slate post-tick 中可同步完成：正常 dialogue、手动候选、实际 Load回HUD、重新dialogue、旧动作重放、新卡确认，直至首次yield等待任务完成。该公开路径会执行 SetInputMode/SetKeyboardFocus/FlushPressedKeys，多次 Refresh 并经真实 HUD snapshot回调 OpenPage(hud)。

frame423输入模式变化与这一时序相容；这仅是缩小复现的假设。没有中间检查记录，不能断言实际已达到freshConfirm，也不能把Prepass栈解释成确定的某个页面/业务API缺陷。日志中的异步资产编译内存估算提示不证明本次读0x3d的根因。无证据归因缓存、内存、并发或模型。

## Root 首次复跑版本

`verify_ui_loadpoint_pie.py` 已增加仪表（264行，AST PASS），原调用顺序/yield保持不变。增量 `ui-loadpoint-progress-only.patch`：

- 启动、各stage及每check立即关闭写出 progress.json；继承fixture的require同样落盘，以区分公共基础夹具与UI路线。
- ExecuteAction、DescribeLayout、ActionAt、LoadPoint 调用前记录operation/before；同步返回后记录returned和monotonic。发生原生crash时能区分“某调用未返回”和“已返回、随后Prepass崩溃”。
- results.json 仍仅正常结束/捕获Python异常时写；progress不是成功证据。
- 不新增yield/延时、不改按钮捕获/布局、原GUID、第一世界行为、库存/任务断言。

这是定位实验。即使仪表版未崩溃，也不能声称生产已修复；磁盘记录会增加运行时间。

## 待定的第二个实验

`ui-loadpoint-slate-separation-after-instrumentation.patch` 独立准备，AST PASS；**未应用首次复跑文件**。仅把各 page切换/directLoad完成后让generator在后续Slate post-tick再继续，正常菜单内部Load之后也留一次tick。保持原页面/Load候选前置和所有断言，不先打开HUD或提前取消候选。

先检查仪表版progress。如果定位到特定动作未返回，再缩小该动作；如果调用已返回随后Prepass崩溃，再串行采用独立分帧实验，比较是否可复现。未启动UE/build/model/HTTP/Git或修改root/Source。

所有Slate pointer/键盘真实事件、physical input、模型HTTP、Python NPC receipt/operation检查仍NOTRUN。
