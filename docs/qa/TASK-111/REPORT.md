# TASK-111 验证记录

基线：main `7f56a2f5`（TASK-110 / PR #75）。工程：`G:/GameFactory/Hearthward/Hearthward.uproject`。工作分支：`codex/TASK-111-navigation-combat`，实现提交 b8239989，整合见 [PR #76](https://github.com/XLingyyy/Hearthward/pull/76)；未发布。

## 实现

- 暂停菜单遵循鼠标／键盘输入状态。刷新保留 Enter 的逻辑默认选项，鼠标位于空白处时不绘制默认焦点。
- 地图打开时从当前真实场景生成 4096² 正交俯视图，沿用既有 XY 投影、缩放、平移与任务定位。使用无光照材质颜色，夜间也可读；仅开图时捕获，平移和鼠标移动不触发捕获。地图新增约 64 MiB RGBA8 渲染目标。
- 玩家为金色方向箭头并标“你”，弟弟为蓝色方向箭头并标“弟弟”；靠得很近时分开显示，通过引线连接真实坐标，避免图标重叠。
- HUD 顶部增加连续滚动罗盘，中央指针固定，刻度和东南西北随镜头朝向移动，两端渐隐。投影约定：+X 为东，+Y 为南。
- 普通近战允许持续移动、冲刺；移动动画提供骨盆和双腿，`spine_01` 以上叠加攻击。处决保留全身动画和移动锁定。弓／弩现有上半身动作允许移动。
- 石斧扫掠使用当前移动姿态的根骨／骨盆，保留原攻击时间窗、武器插槽、伤害和耐久规则。冲刺仍有原来的体力消耗。

## 已验证

- 修改前原生回归复现 5 个失败断言：空白区域刷新后高亮继续（3 次）、攻击前摇／后摇锁定移动。
- 修复后 `Hearthward.UI.Task111.PauseAndMovement` 通过；`Hearthward.Combat.ExecutionActionsAndSnapshot` 通过。前者保留 1 条既有测试夹具 EnhancedInput LocalPlayer 警告。
- Editor Development 编译通过。
- 在独立 GUID 存档池运行实际自然地图，地图俯视图、北／东朝向罗盘、角色标记和移动挥斧截图已检查。首次俯视图低处地形被裁掉，已用明确的 600000 cm 远裁剪距离修复并复测。
- `render-04` 的 11 项检查全部通过：按住游戏视口 W 键、由战斗公开接口发起石斧攻击，攻击期间移动 675.82 cm，最高速度 587.70 cm/s；脚部和手部骨骼在攻击期间分别变化 36.83 cm、26.22 cm，攻击正常结束。截图显示腿部跑步和上身挥斧同时播放。
- 最终 `build-07` 编译、`render-05` 的 11 项检查通过，已目视确认靠近的兄弟标记分开且保留真实坐标引线。最终攻击位移 649.73 cm、最高速度 587.70 cm/s。可移植记录：[原生回归](native-result.json)、[最终运行结果](runtime-result.json)。最终地图截图：`E:/HearthwardQA/TASK-111/render-05/map.png`；罗盘与动作截图在同目录。
- 仓库规则检查与 `git diff --check` 通过。

2026-10-12 最终宽度微调：罗盘宽度 540→486（缩短 10%），X=593，保持居中、字号和顶部高度；`build-09` 编译及局部 `git diff --check` 通过。该次仅修改位置与宽度常量，未重复运行全套渲染；上述运行截图对应缩短前版本。

## 验证边界

2026-10-12 罗盘样式微调：方向字字号 18→15，数字 12→10；540 宽罗盘在 1672 宽设计画布中水平居中，顶部位置 26→40。`build-08` 编译通过，`render-06` 的 11 项检查通过，实际北向截图已确认居中、缩小及下移效果：`E:/HearthwardQA/TASK-111/render-06/compass-north.png`。

测试使用 Development 引擎视口输入与公开 API，并非 Windows 人工键鼠录屏。测试准备包括移到室外、同步装备读取状态、推进测试时间到白天；没有修改玩家正式存档。早期失败运行保留在 `E:/HearthwardQA/TASK-111/`，未计为通过。

本轮未重新打包 Shipping，未逐把武器进行完整命中、连招与手感验收，也未进行全流程或性能基准验收。地图是开图时当前已加载场景的俯视画面；长时间保持地图打开时，场景变化在下次开图更新。既有新游戏测试中的装备读取状态需要显式同步，未将该测试前置操作宣称为正式开场流程验证。

## 重现入口

```powershell
../.venv/Scripts/python.exe -X utf8 scripts/qa/run_engine_gate.py --output E:/HearthwardQA/TASK-111/native-new --tests Hearthward.UI.Task111.PauseAndMovement+Hearthward.Combat.ExecutionActionsAndSnapshot
../.venv/Scripts/python.exe -X utf8 scripts/qa/verify_navigation111.py --output E:/HearthwardQA/TASK-111/render-new
```

输出目录需为新目录。完整构建日志、原生报告和运行截图位于 `E:/HearthwardQA/TASK-111/`。

实现参考：[Epic：Using Layered Animations](https://dev.epicgames.com/documentation/unreal-engine/using-layered-animations-in-unreal-engine)。沿用本项目现有原生骨骼分层节点。
