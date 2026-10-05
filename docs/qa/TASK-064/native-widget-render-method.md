# TASK-064 真实地图 Widget 离屏渲染方法

2026-10-04。仅给既有 `Hearthward.Map064.ExplorationBoundaries` 添加两处 Capture；不改原探索、Fog、路线、标记或任务断言，不新增业务用例。当前 build / 渲染均 NOT_RUN，由根串行执行。证据将标记为原生真实 Widget 离屏渲染和显式 C++ fixture，不计作 PIE、正常键鼠、正式地图通行或 Owner 体验验收。

两文件补丁见 `../TASK-059/native-widget-render.patch`。该测试沿既有真实 Screen 创建、OpenPage(map)、Gameplay tick 和 Refresh；局部 Capture lambda 使用062既有方式：[FWidgetRenderer](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/FWidgetRenderer) →1696×954 RGBA8→PNG，并保存 Screen DescribeLayout。显式 `-Map064Render` 启用；NullRHI或FApp不能渲染直接返回，不创建目录或RenderTarget。

fixture 增加实际 UHearthwardPresentationComponent 并注册，满足 `HearthwardScreenPaint.cpp:228` 的绘制契约。没有生产 fallback、反射 setter、新保存接口、世界 API 或通用 helper。

| PNG 名 | 既有测试状态与调用位置 | 根需核看的内容 |
|---|---|---|
| `map-initial-exploration.png` | 正常 OpenPage(map) 后，既有玩家位置由真实 Gameplay tick 产生一个100m探索样本；Fog内/外断言后 | 初始探索区域显示、外部仍有Fog，地图边界/标题/控件中文正常及不重叠 |
| `map-trace-explored-boundary.png` | main_04已领取，真实Track(main_05)；位于已批准 route_trace 首点，真实tick产生探索，Screen Refresh；所有路线/地点/目标泄露断言后 | 同一地图上已探索小段有路线点、未探索的大段保持Fog且无精确路线/目标标记 |

第二张状态是 route 已/未探索混合：仅首点附近有100m探索；既有正断言要求已探索路线点>0，未探索路线点=0。没有打开全部地图或给未探索区域直接添加路线。画面中的定位来自显式fixture，不能登记成正常玩家走过路线。

输出为 `Saved/Task064/native-widget-offscreen/` 两张 PNG、对应 `*-layout.json` 和 method.json。目标创建或PNG导出失败会 AddError，原业务断言继续执行。

根沿既有公开 UEClient runner 的复跑命令，本轮未执行：

```powershell
& 'G:/GameFactory/.venv/Scripts/python.exe' -X utf8 'G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-053/run_native.py' --filter 'Hearthward.Map064.ExplorationBoundaries' --label render064 --render --extra-arg=-Map064Render
```

报告为 `Saved/Task053/render064/index.json`，runner结果为 `docs/qa/TASK-053/render064-native.json`。两项可在同次根串行引擎会话中合跑：

```powershell
& 'G:/GameFactory/.venv/Scripts/python.exe' -X utf8 'G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-053/run_native.py' --filter 'Hearthward.Camp059.WorkerAssignmentPresentation+Hearthward.Map064.ExplorationBoundaries' --label render059064 --render --extra-arg=-Camp059Render --extra-arg=-Map064Render
```

合跑报告为 `Saved/Task053/render059064/index.json`；三张059、两张064图仍分任务目录输出。根核看五图和 Native 原断言后分别登记，不能把未运行的图像记录写成PASS。

本轮静态检查：原文件所有行保持原顺序、既有测试数量未变、Capture调用2次、渲染/IO位于专用opt-in与非NullRHI守卫之后。没有 UE / C++编译 / 实际PNG信用，没有 Git 写操作；062 的 Source/flag/QA 未修改。
