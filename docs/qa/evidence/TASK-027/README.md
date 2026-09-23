# TASK-027 验证证据

基线 `414dc70481746875c8bd2b36d8b6140b475fcace`，分支 `codex/TASK-027-character-animation`，日期2026-09-23，UE5.8.2。以下均是本地实施证据，不代表独立审查通过。

- [Editor构建](build.json)：Development Editor编译与链接通过。
- [PIE功能报告](verification.json)：替换UE模板后重新运行，24/24通过。`checks`逐项记录断言，`samples`记录实际动画状态、速度和位置。通过Enhanced Input动作注入及现有玩法API操作真实PIE角色。另有行走支撑脚高度断言，防止源骨架缩放遗漏导致离地。攻击恢复检查改为有上限的状态等待，避免在0.7秒片段末帧之前按墙钟时间断言。功能PASS本身不能证明手脚姿态正确，模板替换另做正面/侧面分相位视觉检查。
- [动作录像](animation-review.mp4)：由报告对应的960×720渲染帧以6 fps编码；测试镜头和灯光是临时对象，不保存地图。低帧率录像用于动作辨认，不用于评价游戏帧率。
- [真实键鼠观察](physical-input.json)：Windows窗口输入观察；确认W移动、空格起跳/下落/落地、鼠标左键动作预览。短促Shift组合键未捕获达到冲刺速度的样本，E输入时已离开交互范围；这两项不计为物理输入通过。冲刺按下/释放和资源动作另由PIE功能报告验证。
- [初版动画产物](authored-motion.json)仅记录初版，当前移动三段以 [模板重定向](template-retarget.json) 为准；其余五段未变。[资产检查](asset-inspection.json)、[原八段动画LFS锁](lfs-locks.json)、[模板及重定向LFS锁](template-locks.json)。
- 模板正面/侧面四相位截图位于 `template-preview/`，其中 `SourceIdle_0_front.png` 为原Tripo待机对照；当前待机双手位于躯干外侧，双脚平放。
- 关键帧：[待机](idle.png)、[行走](walk.png)、[冲刺](sprint.png)、[跳跃](jump.png)、[挖掘](dig.png)、[攻击](attack.png)。

## 复现

在 `G:/GameFactory` 使用项目虚拟环境，通过 `engine_adapters.ue5.UEClient` 启动Editor。`map_path`为 `/Game/Hearthward/Tests/Graybox/L_GrayboxValidation`，`extra_args`传入：

```text
-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-027/verify_animation.py
-HearthwardSaveTestPool=<新UUID>
-Unattended
-NoSound
```

脚本创建临时测试资源、场景灯光和捕获相机，运行PIE并退出；结果写入 `Saved/Task027/playtest/verification.json` 与 `frames`。每次生成新的SaveTestPool UUID，避免影响正常存档。`-Task027Interactive`切换为键鼠观察模式，移除旧的 `.agent-local/task027-stop-physical` 标记后启动；创建该标记结束观察。观察模式的`passed`只表示观察正常结束，不能当作所有按键都通过。

自然地图使用既有 `scripts/world/TASK-026/open_rebuild.py --game --medium` 启动。其余五段动作的制作脚本是 `scripts/characters/TASK-027/author_motion.py`。移动三段由 `scripts/characters/TASK-027/retarget_template.py` 制作，启动时另传 `-EnablePlugins=SkeletalMeshModelingTools`，该开关仅作用于本次Editor进程。另传 `-Task027TemplatePreview` 可在生成后捕获正侧面预览。两脚本执行前需持有对应LFS锁。

## 已知限制

未进行打包版或性能验收。基础动作未加入脚部IK、武器/镐模型、握持约束或矿石结算。视觉复核检查了角色比例、朝向、步态和动作切换；不等于完成专业动画精修。仓库校验的5项流程元数据错误见 [交接](../../../handoffs/TASK-027.md)。
