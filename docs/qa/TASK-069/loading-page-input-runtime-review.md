# TASK-069 加载结束后的页面输入恢复

2026-10-05，Root 串行公共 UEClient，UE 5.8.2 Editor Development `-game`；实际 OS 键鼠及独立 UUID 存档池。基线67fb0784ca8c6d488173e587e7f95c4be0d9092a上的当前未提交补丁。无世界夹具、位置设置、资源授予或AI请求。

原Apply路径真实RED：第七进程23232在100% Settings Apply、返回Title、Continue之后，日志20:54:54显示Hide before=0、saved=1。LoadingEnd后实际Escape与Tab均无效，HUD时钟持续推进。见[RED截图](loading-after-settings-input-red-os.jpg)和[观察记录](loading-after-settings-input-red.json)。第六冷Continue诊断saved=0且未完成载入后按键对照，仅作历史记录，未记业务RED。

加载开始快照来自Title的UIOnly；新HUD先设GameOnly，但Hide恢复旧true。修复只在现有OpenPage的SetInputMode之后通知Loading保存页面最新IgnoreInput；遮罩有效时继续临时阻止游戏输入，Hide恢复最新页面目标。没有页面变化的travel保留原快照。没有在FinishSession读取遮罩自身的true，也未增加焦点框架或修改存档。三个生产文件及调用链独立只读复核见[Source复核](loading-input-current-source-readonly-review.md)。

Native单项Hearthward.UI069.LoadingRestoresLatestPageInputMode覆盖Title→HUD恢复false、最后切回Pause恢复true、无切页travel恢复false，1/1成功、0错误、1既有EnhancedInput夹具初始化警告。原稿缺Traversal组件导致夹具首次崩溃，补齐实际组件后通过，未增加生产null fallback；崩溃记录见loading-native-fixture-missing-traversal.json。混合报告[loading-green-map-exploration-font-red-native.json](loading-green-map-exploration-font-red-native.json)中Loading成功、Map独立字号失败，混合运行整体退出1，未登记整体PASS。

第八正常进程43112重复相同Apply100→Title→Continue路径，应用后实际无边框2560×1600；21:08:10 Hide before=1、saved=0、LoadingEnd7.01秒。加载结束后实际Escape打开Pause，Escape回HUD后Tab打开Inventory。见[Pause截图](loading-after-settings-pause-green-os.jpg)及[Inventory截图](loading-after-settings-inventory-green-os.jpg)。可见存档为等级2、经验50/135、护符1、石斧80/80、负重20.5/100；只比较这些可见状态。

返回标题确认时Hide保存1，标题输入仍有效。实际Quit确认后日志21:10:48 RequestExitWithStatus(0,0)，21:10:49 Preparing to exit；Root确认PID43112消失、游戏窗口消失之后才写finish marker，随后host0、公共stop_ok。见[完整host结果](normal-os-28258bcb-results.json)，launch/stop/finish同编号保留。本项正常输入恢复与自然退出通过。

本次-NoSound，不提供声音、全部九页/分辨率/Windows DPI、正常倒地、Owner体验、完整存档权威比较或Shipping信用。Task仍Active。
