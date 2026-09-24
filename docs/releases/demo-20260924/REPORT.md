# Windows Demo 0.1.0 发行记录

发行目标：[v0.1.0-demo.20260924](https://github.com/XLingyyy/Hearthward/releases/tag/v0.1.0-demo.20260924)。用户授权构建并公开发布 Windows 可玩 Demo，范围见 [SCOPE.md](SCOPE.md)。

基线 main：`68e68d817c4d5a39bf43eb7f27863fc734d04aef`，分支 `codex/demo-release-20260924`。发行源码完整 SHA 记录于 Release 正文与随包 BUILD-INFO.json；不合并 main。UE 5.8.2 / Win64 Shipping / RTX 4060 Laptop 8 GB / Windows 本机验证。

## 发行修复

- 修复 Shipping 中 checkf 被移除导致 UI 布局加载、NPC policy JSON 解析均未执行的问题；已复现黑屏后验证标题与游戏正常。
- NonUFS 部署 NPC policy、UI、玩法数据、本地 Qwen3.5-4B Q4_K_M、llama.cpp CPU/Vulkan 运行库与许可。动画和动态加载设施目录显式 cook，启动地图为 Bootstrap，自然地图随包部署。
- 禁用 Zen cooked store，使用本地 cook / Pak / IoStore。15 个 TASK-028 材质的 Roughness TextureSample 按 TC_MASKS 修正为 Masks；逐资产变更见 [material-fixes.json](material-fixes.json)，保留 LFS 锁待集成。
- 默认中等画质、30 FPS。原 Epic/不限帧和 Epic/60 FPS 下本机 GPU 满载，真实模型委托超时；中等/30 FPS 同一请求返回有效的两份木材任务卡，确认后实际采集入库完成 2/2。此为本机对照结果，不承诺所有硬件响应时间。
- Inno Setup 6.4.3 分卷安装，附带 VC++ 运行库；桌面 GPU AI 入口、开始菜单 CPU AI 入口。安装无需 UE、Python、API Key 或另行下载模型。

## 验证

- Win64 Shipping BuildCookRun：通过；最终结果见 [package-result.json](package-result.json)。材质编译错误为 0。
- 独立包物理输入：标题、新游戏、继续游戏、双角色显示、移动、透明背包/对话、装备石斧、真实 Qwen 委托、确认、NPC 采集交付 2/2、仓储木材 2、取出 1 到玩家背包、F6 手动保存通过。
- 安装器：从完整 exe + 三个 bin 安装到独立目录成功，无需重启。
- 仓库工具单测：31 项通过。
- 仓库自检：已有 TASK-027 / TASK-028 缺 reviewer 与实际 Issue URL，共 4 项元数据错误，未改动或跳过这些检查。
- 最终新用户配置及安装包启动复核：待完成。

## 复现

从上层 GameFactory 根目录，以其 .venv Python 运行 `Hearthward-ai-npc-fix/scripts/release/package_demo.py`。引擎操作使用 UEClient public API，所需适配器扩展保存在 [ueclient-packaging.patch](ueclient-packaging.patch)；先将补丁应用到对应 GameFactory 版本。模型按项目 `config/local-ai.lock.json` 准备到 Runtime/LocalAI，不进 Git。

将 `scripts/release/README-DEMO.txt` 复制到 archive Windows 根目录，以 Inno Setup 6.4.3 编译 `scripts/release/demo.iss`（PackageRoot / OutputRoot 可用 ISCC /D 覆盖）。安装包排除 PDB 和 staging manifest，不包含开发者存档。独立运行测试使用 `scripts/release/play_demo.py <Windows目录> <隔离用户目录>`，通过 UEClient.launch_packaged 启动。

## 边界

本版为 prerelease 可玩预览。完整剧情、自然地图战斗敌人、正式关卡尚未完成；武器挂接、跳跃手臂、地形和植被仍有视觉瑕疵。不同硬件、显卡驱动及无开发环境的第二台电脑测试为 NOT_RUN；本机独立安装与随包模型已实测。建议 16 GB RAM、8 GB VRAM 和 10 GB 可用磁盘属于体验建议，并非完整兼容性认证。
