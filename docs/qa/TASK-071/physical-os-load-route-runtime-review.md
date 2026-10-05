# TASK-071 实际键鼠交流取消与读档确认

2026-10-05，Root 操作，UE 5.8.2 / Windows 11 / D3D12 / RTX 4060 Laptop。集成树 `task051`，分支 `codex/TASK-053-traversal`，HEAD `67fb0784ca8c6d488173e587e7f95c4be0d9092a` 加当前未提交施工；受测源码包含 069 持久 EditableTextBoxStyle 修复与 Enter 安全确认。没有提交、推送或 Owner 最终验收。

## 实际结果

隔离池 `8618582c-c61b-4686-b19f-0e80f6f8a7ba`，Editor PID 48088；21 个夹具前置全部通过。Root 使用 computer-use 的 sky 真正操作 Editor 游戏视口，监视器只读取公开状态和布局，hold 期间没有调用 UI 动作、加载或模型提交。

- 实际按 T 打开交流页，鼠标点 Draft 后观察到蓝边和光标；输入“仅检查输入，不发送 E R”，没有发送。实际 Escape 回 HUD；未发生此前首次 dialogue 的 Slate 崩溃。
- 实际 F6 打开存档页，鼠标选手动基线 A，弹出确认。按 Return 后弹窗保持，epoch 和 player wood=1 均未改变。
- 实际 Escape 取消，仍为同一存档页和原 epoch，player wood=1。第一次弹窗 layout sequence 226/sample 1356，到取消 sequence 227/sample 1400，约 22 秒没有 authority/epoch 变化。
- 再次鼠标选同一手动 A，然后鼠标点“确认”。sample 1449/sequence 229 回 HUD，epoch 唯一更替，player wood 1→0，source wood=80、camp wood=20、brother wood=0、requested/delivered=0 和原实例保留。截图实际显示恢复反馈。

1451 个 0.5 秒采样、6 个状态事件、230 次布局变化，零模型调用、零模型进程。Host 最终 `CAPTURED`，monitor error=null，独占 PID 已正常关闭。原始报告保留为本目录 `physical-os-8618582c-*`，完整 Saved 目录另含 cases；完成 marker 只结束记录，监视器 acceptance 始终 NOT_EVALUATED。

## 证据边界与发现

以上局部 OS 路线 PASS，由实际操作截图和公开状态共同支持。DescribeLayout 不记录 Draft 文本或焦点，单独 journal 不证明按键来源；0.5 秒采样也不证明每个瞬态 Paint。type_text 中文输入没有覆盖 IME 组合输入。F11 没有观察到全屏变化，不记成功。

这是显式 prototype 灰盒，包含临时地板、隔离战斗目标、source_safe、公开物资授予、测试营地/伙伴及停用 gameplay tick，完整披露见 setup/results。没有覆盖正常新游戏、九页完整流程、右 Ctrl/Shift、正常退出后 Continue、旧 Slate 回调或真人体验。期间 sample 1199 自动产生第三个未锁定自动节点，不能归因于打开 F6；读回选的是原手动 A。

实际交流页在亮灰地面上的文字和 Draft 缺少足够深底，已单独调查可读性；本报告不把交流页面整体视觉记为通过。
