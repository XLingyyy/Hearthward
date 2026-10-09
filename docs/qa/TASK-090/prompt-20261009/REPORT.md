# TASK-090 交互提示回归修复 · 2026-10-09

用户实测：所有交互点靠近后缺少交互视觉提示；卧室遗物仅显示任务标记。
根因：新版 Screen 启用时 legacy HUD 提前返回，而 ComposeHUD 只接入完成反馈，漏掉附近目标提示。历史删除来自 4e615af1。

修复：HUD 读取原工作台、Campaign 和通用 Interaction 的实际目标/距离，按原交互优先级显示持久提示；离开范围立即随 HUD 刷新隐藏。Campaign 新增只读 InteractionPrompt，保留原 Prompt 行为。按键跟随设置，仓储/交互换键互不污染；底部暗色衬板高度跟随真实换行与字号。倒地、忙动作和建造预览继续优先。没有改交互距离、经济、奖励或存档格式。

扩展回归复现 Element 的通用前缀替换会二次映射语义按键：仓储 E / 交互 F 被画成 F / F。当前提示在映射后直接设置文本，保留其他页面旧行为；key-remap-red-index.json 保留该断言失败。

原生 RED：relic/改键/重复刷新/generic 四个显示断言失败，实际目标检测通过；首轮另有夹具缺少 SetIsFocusable 的 1 error/1 warning，后续补齐，原四个回归断言保留。
首次 RHI 截图夹具分别缺少 SurvivalComponent 和 PresentationComponent，在 NativePaint.cpp:59/574 访问空组件而崩溃；正式角色拥有该组件。已修正夹具，未改生产渲染器，失败 result 保留 render-fixture-failure.json。
最终 green-final-render：Task090 4/4 Success，新增 NearbyInteractionPrompt 0 error/warning；两条既有倒地用例各 1 EnhancedInput 夹具 warning。UE 启动时独立于测试会话有 13 条 Condition failed 日志，发生于 Engine Initializing 前，保留完整 result，不计作本单测试断言。未声称进程完全无错误日志。
覆盖：遗物实际检测和显示、离开范围/旧反馈、重复刷新、改键、150%文字、普通床、仓储双键、工作台与遗物同时在场时的优先级、建造预览隐藏、菜单隐藏、只读查询不发奖。实际 FWidgetRenderer 导出 1280×720 两张 150% HUD 图。
独立 PIE：正式地图新游戏，定位到实际遗物附近，正式 HUD 可见；离开到 4 米后提示消失。pie-near-hud.png / pie-far-hud.png 为该 PIE 的实际 Widget 绘制；near-world.png 为不含 Slate UI 的场景截图；测试位移为 API 设置，不计正常键鼠路线验收。独立测试 UserDir/SaveTestPool，不改用户存档，不保存关卡或资产。

构建：UEClient Development Editor 成功。原始日志保留 .agent-local/qa/TASK-090/。源码状态由对应提交及 candidate10 构建快照绑定；Shipping 打包状态见 candidate10 BUILD_INFO。完整 084—103 批次验收与此前模型/性能/真人未完成项不由本修复代替。

范围检查：scripts/validate_repo.py --task TASK-090 --base 104842d0 实际 PASS（0 errors），该基线是本次已提交的范围快照。
复跑：公开 UEClient.build.project(target=HearthwardEditor, configuration=Development)；testing.run_automation_tests 的筛选为 Hearthward.Iteration.Task090.，RHI 截图参数为 -HearthwardPromptCapture，附独立 -UserDir 和 -HearthwardSaveTestPool。最终报告 green-final-render-index.json，测试进程退出码 0。

候选10实际 Shipping Build/Cook/Stage/Archive 成功，源码 0e430f5fa740dff35bd1ba8924e031115edb9cae，版本 0.2.0-preview.20261009.2。运行目录 F:/HearthwardDemo/iteration-084-103-20261009-10/Windows；两个启动器、模型/运行库/资源/许可文件已安装并检查存在。package-result.json 保存公开 UEClient 实际成功结果。源码已推送至 codex/TASK-084-103-iteration；本包正常键鼠操作仍未验，不沿用候选9信用。
