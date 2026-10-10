# Hearthward v0.3.0 发行记录

2026-10-10用户明确授权提交、推送、创建PR、合并main，并打包发布v0.3。版本使用v0.3.0/0.3.0以兼容游戏现有三段式比较。正式[发布页](https://github.com/XLingyyy/Hearthward/releases/tag/v0.3.0)的附件与状态为发布结果；本文件记录本次实际构建与操作。

## 源码与构建

[PR #68](https://github.com/XLingyyy/Hearthward/pull/68)通过repo-policy后合并。干净main构建提交：`5f389a82ad6bd3f58b3a26a450a198ed91fe2f16`。随后的发行记录只补文档和证据，不改游戏代码、配置或资产；发布标签包含这批发行文档，游戏源码与资产保持上述实际构建版本。

- Win64 Shipping Build/Cook/Stage/Archive成功，AutomationTool退出码0，用时约9分50秒；[构建结果](package-result.json)、[构建身份](build-info.json)。Cook为0 errors/1 warning，该提示来自Agent Integration插件的UE技术许可说明，不记为零警告。
- 使用既有UEClient公开API；输出 `F:/HearthwardDemo/v0.3.0/Windows`。保留旧候选与玩家存档。
- [全部65项工具测试](tools-tests.txt)通过。[本机仓库检查](repository-check.txt)0错误。首轮PR CI发现4个历史本机QA链接；已将其改为明确“仅本机留存”，后续CI通过，没有跳过检查。
- 本版包含此前候选14与[099/104整合](../../qa/TASK-104/integration-20261010/REPORT.md)。该整合快照的Editor构建、099原生30项、104原生9项与095回归1项均有通过结果；原生结果详见原报告，不改写成Shipping测试。

## 独立包实际操作

通过Computer Use真实键鼠操作发布包，使用独立 `F:/HearthwardQA/v0.3.0/profile`。

1. CPU后端启动，[标题](title.png)显示0.3.0；Return新游戏进入正式[石堡卧室](new-game.png)，角色、弟弟、HUD和任务标记加载。
2. Tab打开[背包](inventory.png)，M打开[地图](map.png)。F6进入存档页后实际点击保存，[存档节点由1变2](save.png)。
3. 停止该进程，以Vulkan后端新进程启动；鼠标点击继续，恢复[同一场景与进度](continue.png)，[原手动/自动两节点保持](restored-saves.png)。
4. 两个自有进程均由原UEClient关闭成功，见[CPU启动](cpu-launch.json)/[关闭](cpu-stop.json)、[Vulkan启动](vulkan-launch.json)/[关闭](vulkan-stop.json)。

[观察记录](smoke-observation.json)限定上述范围；未执行实际本地模型推理请求，CPU/Vulkan参数启动不代表语言质量测试。

## 发布文件

完整ZIP：`Hearthward-v0.3.0-Windows.zip`，4,662,237,325字节（约4.66GB）。199个运行文件；[包检查](runtime-validation.json)确认26个声音事件、31份引用WAV、地面表、GGUF、CPU/Vulkan运行库和必要许可文件齐全。公开树排除实际存档、私有配置/QA、源码及制作提示文件，文本秘密模式扫描通过。没有以许可文件存在代替TASK-094全部来源验收。

ZIP文件名/大小目录与运行树一致；3个连续分卷总大小等于ZIP，详情见[分卷记录](bundle-result.json)。

| 下载附件 | 字节 |
| --- | ---: |
| Hearthward-v0.3.0-Windows.zip.001 | 1887436800 |
| Hearthward-v0.3.0-Windows.zip.002 | 1887436800 |
| Hearthward-v0.3.0-Windows.zip.003 | 887363725 |
| Combine-Game.cmd | 791 |
| DOWNLOAD-README.txt | 552 |

下载全部分卷与Combine-Game.cmd至同一目录，运行脚本还原ZIP，解压后在Windows目录运行“启动游戏.cmd”；CPU备用为“启动游戏_CPU.cmd”。保留完整目录，无需UE、Python或另外下载模型。建议预留20GB空间。

## 保留限制

完整剧情和连续路线按用户此前要求后置；设备实际混音、全战斗/采集握姿、旧档兼容矩阵、DPI矩阵、完整性能与第二机器仍未验收。既有语言理解质量与1% Low性能未达原门槛；主角持斧小指较松等视觉限制保留。发布不自动把TASK-084—104改为Done，也不代填人工/来源验收。
