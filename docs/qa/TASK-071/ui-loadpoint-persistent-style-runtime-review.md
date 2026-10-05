# 公共 UI 回调跨读档与样式寿命：实际 RED→GREEN

2026-10-04；Root 串行 UEClient，Editor Development 构建通过。受测为未提交集成补丁。执行 `run_ui_loadpoint_pie.py --label persistent-draft-style-green`，独立档池 aeddf2b6-3362-4074-8cf3-b96a73451b09；全部 96 项检查通过，stage=done、error=null，编辑器正常关闭。

此前实际失败三次。第一次及 instrumentation 版在卡片确认后的下一 Slate tick 原生崩溃；分帧版进一步缩小到第一次 OpenPage dialogue 返回后的下一 tick，尚未创建卡或调用 Load。独立只读源码审阅确认 Refresh 的局部 FEditableTextBoxStyle 经过本机 UE5.8 UEditableTextBox::SetWidgetStyle(&InStyle) 路径被 SEditableTextBox 保存地址，布局/字体随后解引用悬空栈地址。保留三个 crash RED 的日志与 progress。单行生产修复将局部副本改为 Draft->WidgetStyle 的引用，使用控件自身持久 UPROPERTY；保持 Paint 和其他操作不变。

同一真实分帧路线完成两段：第一段实际旧手动委托卡跨 Load，旧 ActionAt 的确认 GUID 无效，新卡旁重放旧 ID 不影响新候选，正常公开 UI 确认的新任务真实完成，camp木材+1、S1资源-1、其他背包不变。第二段通过正常 UI 保存不同时间线 B，旧菜单 A 加载确认跨实际 LoadB 后无效，重新进入菜单并取得新确认可实际恢复 A；epoch/HUD/库存/当前公开任务全部正确，安静观察后无额外影响。

这是 DescribeLayout→实际 ActionAt→ExecuteAction 的公共 UI 回调重放，分帧允许真实 Slate layout/paint 运行。slate_event_replay 与 physical_input 仍 NOTRUN，不能计作桌面物理输入或直接 Slate 鼠标/键盘事件验收。全程 model_generation_calls=0、server_process_id=0；模型 HTTP 跨读档已在独立 http-loadpoint-runtime-review.md 记录。没有公开 Python receipt API，本路线不登记回执断言。

TASK-069/071 仍 Active；正常退出继续、强制成功迟到 HTTP、物理输入与完整 Owner 验收另行保留。
