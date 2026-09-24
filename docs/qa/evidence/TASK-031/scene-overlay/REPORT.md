# TASK-031 背包与对话场景透显

2026-09-24，Windows，UE 5.8.2，HearthwardEditor Win64 Development。基于 e5ac3ed 后的本次 UI 补丁；完整受测代码 SHA 见 tested-revision.json。

背包与伙伴对话的页面 background 清空，绘制层不再填充全屏黑底；背包装饰人物插画隐藏。删除对话页关闭 viewport 世界渲染及相关恢复代码。物品格、装备、属性、对话输入及按钮保留。背包保持暂停；对话不暂停且持续渲染世界。其他页面行为不变。

## 验证

- Editor Development 构建 PASS，见 build-result.json。
- Bootstrap 新游戏进入自然地图，16/16 定向 PIE 检查 PASS，见 result.json。覆盖装备、输入、记忆子页返回、重复打开/关闭及暂停状态。
- 实际 `Shot SHOWUI` 截图人工检查：[背包](inventory.png)、[对话](dialogue.png)、[对话内转动测试视角](dialogue-turned.png)。截图包含真实 Slate 与场景；最后两张验证对话打开期间世界渲染更新。测试通过控制器 API 转动视角，没有新增窗口内鼠标视角操作。
- 使用真实页面处理函数；本轮未做物理键鼠、LLM 推理耗时、Shipping 或第二机器验收。

## 复现

使用 GameFactory Python 环境执行 `docs/qa/evidence/TASK-031/run_validation.py build`，然后执行同脚本 `overlay`。先归档已有 `Saved/SceneOverlayValidation/result.json`。脚本通过 UEClient 启停自己的编辑器，采用独立测试存档池，不覆盖玩家存档。实际本轮使用 Saved/NPCValidation/run_validation.py 的等价 UEClient 入口执行 verify_scene_overlay.py。

接口依据：[Epic UGameViewportClient API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameViewportClient)，bDisableWorldRendering 控制世界绘制。

仓库元数据校验中的 TASK-027/028 Reviewer 与 Issue 缺项为既有问题；用户本地工程关联和 IDE 文件不进入提交。
