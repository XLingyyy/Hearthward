# TASK-026 当前验证证据

证据时间：2026-09-21 23:35（Asia/Shanghai）。受测工作树基线：`b1f85525697b79e6017455decab9d79a54977834` 加本任务未提交资产；实现提交SHA在交接文件中补记。

## 执行方式

本机使用 UE 5.8.1 编辑器打开任务地图并执行：

```text
UnrealEditor.exe Hearthward.uproject /Game/Hearthward/World/Natural/L_NaturalWorld \
  -windowed -ResX=1280 -ResY=720 -ForceRes -nosplash \
  -ExecutePythonScript=docs/qa/evidence/TASK-026/verify_natural_world.py
```

脚本先在编辑器中重开地图、检查资产与依赖，再进入真实PIE，通过Enhanced Input注入现有移动动作。完整布尔结果位于 `verification.json`；创建参数位于 `creation.json`，地图级隔离设置与浅滩修正分别位于 `browse-game-mode.json`、`raise-fords.json`。

## 结果

- `verification.json`：`passed: true`，27/27项检查通过；地图可重开、175个外部Actor包存在、项目依赖均位于TASK-026拥有路径。
- PIE隔离检查通过：仓储木材为0、本地模型进程ID为0、没有保存伙伴夹具。
- 出生点普通移动和西侧浅滩各连续移动超过10 m；地图使用现有第三人称Pawn和现有Enhanced Input。
- 5张1280 × 720观察点截图由同一轮PIE生成：`meadow-route.png`、`river-ford.png`、`forest-sentinel.png`、`hills-twin-fangs.png`、`river-crown.png`。

## 证据边界

本轮没有验证主环线/两条支路/第二浅滩的连续通行，没有Standalone流送、性能采样、打包、干净克隆、第二机器或独立评审。截图显示灰盒运行状态，但也暴露悬空树冠、倾斜树干、重复基础形体、地表拼接和空白区域；因此不构成A4视觉验收。

`Saved/Logs`中的原始引擎日志未纳入Git；受版本控制的JSON报告由验证脚本直接写出后原样复制。未来复测应保留新的JSON、截图、运行命令、硬件和实现提交SHA，不用本轮PASS替代修改后的验证。
