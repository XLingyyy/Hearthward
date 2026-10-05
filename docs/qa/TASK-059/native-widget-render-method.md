# TASK-059 真实营地 Widget 离屏渲染方法

2026-10-04。仅给既有 `Hearthward.Camp059.WorkerAssignmentPresentation` 添加三处可选 Capture；没有新增业务用例或修改原断言。原生 Widget 的离屏图像证据，显式 C++ fixture；不计作 PIE、键鼠流程、真实地图或 Owner 体验验收。当前 build / 渲染均 NOT_RUN，由根串行执行。

补丁：`native-widget-render.patch`，只改 `Source/Hearthward/Tests/CampLaborPresentationTests.cpp` 和 `MapGuidanceTests.cpp`。不要整文件覆盖根的其他任务改动。

Capture 使用既有062方式：[官方 FWidgetRenderer](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/FWidgetRenderer) 的 DrawWidget 把实际 `Screen->TakeWidget()` 绘入1696×954 RGBA8目标，再用既有 UKismetRenderingLibrary ExportRenderTarget 输出 PNG，并保存同一状态的 DescribeLayout。仅显式 `-Camp059Render`、未带 `-NullRHI`、FApp::CanEverRender 时执行。常规 Native/NullRHI 路径在创建目录/RenderTarget 前返回。

两个 fixture 均增加实际 UHearthwardPresentationComponent 并注册：现有 `HearthwardScreenPaint.cpp:228` 在绘制时无条件取该组件的字幕，原数据/layout断言未触发真实 Paint。该补充满足既有生产组件契约，没有增加生产 fallback，也没有手动调用 BeginPlay/改保存 API。

三处截图沿现有真实 Camp 命令、Screen OpenPage / ExecuteAction 和 Advance 状态：

| PNG 名 | 截图时的既有状态 | 根需核看的内容 |
|---|---|---|
| `workers-normal-7-0.png` | camp → workers → actual wood region；5身体、7.0工效 | 标题/岗位/工效/人员列表、按钮中文正常及不重叠 |
| `workers-severe-hunger-6-1.png` | 玩家真实 severe hunger，使3.0变2.1；5身体、6.1工效 | 身体岗位仍5/5，工效呈现6.1，控件不溢出 |
| `workers-offsite-4-0.png` | 玩家离岗，其工效0；5身体、4.0工效 | 工效4.0与保留岗位同时可见 |

输出：`Saved/Task059/native-widget-offscreen/` 中三张 PNG、对应 `*-layout.json`、method.json。PNG 导出缺失/空文件或目标创建失败会 AddError；原身份、生产时钟、有限资源和全部业务断言继续执行。方法未改时钟或业务状态来制造截图。

根在 task051 用已有公开 UEClient runner 执行，命令只给复跑方法，本轮未执行：

```powershell
& 'G:/GameFactory/.venv/Scripts/python.exe' -X utf8 'G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-053/run_native.py' --filter 'Hearthward.Camp059.WorkerAssignmentPresentation' --label render059 --render --extra-arg=-Camp059Render
```

`--render` 使既有 runner 不添加 NullRHI；报告为 `Saved/Task053/render059/index.json`，runner结果为 `docs/qa/TASK-053/render059-native.json`。请绑定实际命令/当时版本、PNG时间和 Native 结果，逐张看图后才登记图像结果。保留普通 NullRHI 原有语义回归，不能用它表示已渲染。

本轮静态检查：原文件每一行按原顺序保留、既有 IMPLEMENT_SIMPLE_AUTOMATION_TEST 数量未变、Capture 调用3次、所有 RenderTarget/文件操作位于专用参数及非NullRHI守卫之后。没有 UE / C++编译 / 实际 PNG 信用，没有 Git 写操作。
