# 自然营地 AI 玩法接入｜2026-09-23

## 版本与范围

- 工作树：`G:/GameFactory/Hearthward-ai-npc-fix`，分支 `codex/ai-npc-vnext-rework-01-fix`。
- 基线：`bf5fb97eb466d38aeef7b6f092a2c6822926e6e1`，加本次接入提交的补丁。原 `Hearthward` 角色动画工作区保持原状。
- Owner 本轮指令：“旧的东西就应该纠正并更新，现在先把已有的AI玩法融入游戏中吧”。据此接入现有玩法并更新当前脚本入口、README、营地记录与任务路径范围；不新增未定经济规则。
- Windows、UE 5.8.2、RTX 4060 Laptop 8 GB；Qwen3.5-4B Q4_K_M、锁定 llama.cpp b10964，Vulkan 32 GPU layers。复用原工作区 `Runtime/LocalAI`，未更换模型、运行库或密钥。

## 可玩的闭环

标题页新游戏进入 `L_HearthwardWilds`，在出生营地创建一个弟弟、营地仓储和 16 份有限木材点。T 打开对话，模型生成任务卡后由玩家确认；弟弟用真实导航采集、携带并入库，沿用世界校验、去重回执、认知与记忆。Z 等待、X 跟随；营地自主活动仍由现有 Routine 执行。R 取仓储材料，B 建造工作台，工作台支持现有玩家制作/维修与 NPC 制作流程。地图/日志和旧开发场景敌人不自动搬入自然地图。

保存同时覆盖伙伴位置、委托、携带物资、资源余量、认知、建筑与玩家状态。Schema 3 新增 `NaturalCompanion` 标记，旧自然地图节点缺省 false，加载时补入营地伙伴初始状态并保留原物资；新节点走正常完整恢复。

自然地图动态导航使用 Navigation Invokers，仅生成玩家、弟弟和营地附近已加载地形的 tiles；边界覆盖地图尺寸。采用引擎已有机制，参考 [Epic Navigation Invokers 文档](https://dev.epicgames.com/documentation/unreal-engine/using-navigation-invokers-in-unreal-engine)。未修改 `.umap/.uasset`。

## 验证结果

| 范围 | 结果 | 证据 |
|---|---|---|
| 最终 Editor Development 构建 | PASS | [build.json](build.json) |
| 原生自动化 | 42/42，无失败、无测试警告 | [native.json](native.json) |
| Python 工具测试 | 31/31 | 命令 `python -m unittest discover -s scripts/tests -v` |
| 自然地图真实模型采集/中途存读档 | 22/22 | [collect-save.json](collect-save.json) |
| 采集10木材→仓储取8→建工作台→NPC制作4箭→重开地图读档 | 23 项通过 | [workshop-and-legacy-harness.json](workshop-and-legacy-harness.json) |
| 旧自然地图真实文件升级与重存 | 12/12 | [legacy-runtime.json](legacy-runtime.json) |

原生、存档及工作台测试在最后两个 UI 修正前运行；后续改动限对话状态显示和不可见场景绘制，使用真实键鼠闭环定向验证。工作台报告整体 `ok=false`：23 项工作台断言全部成功，后续旧档测试使用 Python 访问受保护属性报错。随后改用原生测试生成的旧格式文件，独立 legacy-runtime 全部通过；不将旧整体失败报告伪装为完整 PASS。首次脚本误用 `Guid.is_valid()` 的失败保留在 [attempt01-guid-harness-error.json](attempt01-guid-harness-error.json)。

运行入口：[verify_natural_camp_pie.py](../verify_natural_camp_pie.py)。从 Bootstrap 开始，独立 `-HearthwardSaveTestPool=<UUID>`，默认验证采集存档；`-HearthwardCampWorkshopTest` 测工作台；`-HearthwardCampLegacyTest` 测旧档（先运行原生 `Hearthward.Save.NaturalLegacyFile` 生成测试文件）。启动/停止与构建使用 GameFactory `UEClient` 公开 API。

## 对话渲染复现与修正

可见编辑器中，初次“帮我采集两份木材带回营地。”请求加载约 10 秒后，生成达到 120 秒超时，未产生候选或改写物资。GPU 场景渲染接近满负载；同一输入将帧率临时限制为 30 后，约 18.2 秒返回正确卡片并完成采集入库。[完整失败与重试轨迹](ui-before-render-fix.json) 保留两次请求。

对话使用不透明插画覆盖视口，现进入对话时停止三维场景绘制，保留模拟和 Slate；离开对话及控件销毁时恢复。另修正 Routine 状态被错误显示成“委托受阻”并遮住模型加载状态的问题。

最终重新启动可见编辑器，未施加 `t.MaxFPS 30`，真实键鼠新游戏→T→输入→Enter：加载 7.734 秒、生成 3.500 秒，共 11.234 秒返回正确 collect/wood/2 卡片；确认后保持对话打开，真实模拟完成交付，资源 16→14、仓储 0→2。Esc 返回 HUD 后自然场景恢复。见 [最终键鼠轨迹](ui-final.json)、[任务卡截图](dialogue-final.png)、[返回世界截图](gameplay-final.png)。这是一轮定向复测，不能据此保证所有硬件/输入均无超时。

## 边界

- 当前是现有玩法在真实自然地图营地的接入。角色、资源点、仓储、工作台仍使用开发灰盒外形；没有接入原工作区尚未提交的角色动画。
- 木材点有限 16 份，沿用现有原型参数；未将全图树木变成可采集对象。
- 全图长距离跟随、跨 World Partition 流送返营、敌人分布与战斗、剧情/任务日志、Shipping、第二机器和视觉验收均未完成。
- 编辑器仍有已有 Nanite/SM6 设置提示；启动自动化初始化前有已记录的 `LogAutomationTest` 条件失败，42 项正式测试报告无失败。这里没有宣称全部引擎日志无告警。
- 基线32例真实模型与其他专项结果见[接入前复验](../revalidation-20260923/REPORT.md)，不混算为本次自然地图全功能覆盖。

## 仓库状态

README、交接及 TASK-029 当前任务单已更新。本次成果按用户后续授权提交并推送到既有任务分支，不合并 main。基线任务单仅允许原 AI 栈路径；按基线执行范围检查会将本轮用户授权的 Gameplay、Interaction、导航配置、当前测试入口和营地记录报为 OUT_OF_SCOPE。保留该结果，不修改验证器绕过旧范围；当前任务单记录了本轮授权增量。

仓库通用自检 0 errors，`git diff --check` 通过。基线范围检查实际为 12 项 OUT_OF_SCOPE，详见 [scope-check.txt](scope-check.txt)。

2026-09-23 推送补充：用户确认 main 已替换角色模型。本次 fetch 确认 `origin/main@ba547c0` 合入 PR #35（角色与移动动画）；本地灰盒为 AI 分支尚未同步该提交的预期差异，不作为本轮玩法接入缺陷。
