# 实际 UMG 页面刷新对照

基线 main@2d5c3bbdadf48e37866e40bb826b875a6f1d2235 加 TASK-109 测量代码；同机 UE 5.8.2 Editor Development，真实 Slate viewport / UMG，离屏渲染，独立存档池。每页连续调用实际 Refresh 50 次。计时单位毫秒。

| 页面 | 改前 median / p95 | 改后 median / p95 |
| --- | --- | --- |
| title | 0.133 / 0.175 | 0.135 / 0.251 |
| inventory | 0.908 / 1.022 | 0.607 / 0.773 |
| journal | 0.428 / 0.628 | 0.283 / 0.312 |
| map | 0.174 / 0.263 | 0.133 / 0.151 |
| hud | 0.482 / 0.568 | 0.322 / 0.372 |

保留 title 的 p95 上升；单次进程、每页50次的微测量不能证明全游戏帧率提高。fixture_and_theme 包含创建测试世界和 viewport，总时长改前1129.349、改后936.104，不能称为纯主题冷启动成本。

实际改动：按键名称替换对每次 Refresh 只构造一次，替换表只保留当前页实际改绑的按键；每段文本复用该表。保留每0.2秒刷新与所有状态更新入口。5个日志页高亮的裁切/透明度计算移至离线脚本并保存PNG，运行时普通资产加载。没有关闭HUD、倒地、工作计时或读档更新。

本次运行 UIProfile + Task090 + UI069 + Map064 共11项通过，9项有夹具 PlayerInput 警告，日志原有13项 Condition failed 诊断另存，不能表述为无警告。高亮 `--check` 通过。

完整原始报告位于本机 `E:/HearthwardQA/TASK-109/ui-baseline/native/index.json` 和 `E:/HearthwardQA/TASK-109/ui-optimized/native/index.json`。物理窗口、IME及最终整合结果另记，不由本表替代。

## 实机发现的日志重叠

2026-10-11 在隔离的 M3 旧 QA 存档中，通过真实窗口 J / E 打开日志与支线页，观察到 `side_01` 当前目标重复两遍，侵入任务详情标题。根因是 Preparation.NextStep 默认就是静态 objective，ComposeJournal 又无条件拼接了一次。

修复后当前目标只显示当前步骤；当步骤与完整目标不同时，详情保留完整目标。补充 `JournalObjective` 原生回归，通过实际 UMG 生成 1920×1080、100% / 150% 字体四张图片并逐张查看，主线和三行救援目标均无重叠、文字保留完整。3 项定向原生测试全部通过，1 项有 PlayerInput 夹具警告；报告与图片在 `E:/HearthwardQA/TASK-109/journal-model-v3-fixed`。这不替代中文输入法或所有页面验收。
