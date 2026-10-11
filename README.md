# Hearthward（归火）

UE 5.8.2 单人第三人称生存冒险。当前游戏版本为 **0.3.0**；M0 工程优化已通过[PR #70](https://github.com/XLingyyy/Hearthward/pull/70)合入 main，发行构建提交为 `5f389a82ad6bd3f58b3a26a450a198ed91fe2f16`。

[下载 v0.3.0](https://github.com/XLingyyy/Hearthward/releases/tag/v0.3.0) · [发行与验证记录](docs/releases/v0.3.0/REPORT.md) · [项目状态](docs/PROJECT_STATE.md) · [开发入口](docs/START_HERE.md)

## 下载与运行

在发布页下载全部三个 `Hearthward-v0.3.0-Windows.zip.001`、`.002`、`.003`，以及 `Combine-Game.cmd`，放在同一目录。双击脚本合并ZIP，完整解压后进入Windows目录，运行 **启动游戏.cmd**。CPU备用入口为 **启动游戏_CPU.cmd**。

完整ZIP约4.66GB，含本地AI模型与CPU/Vulkan运行库，无需安装UE、Python或另行下载模型。建议预留20GB空间；保留完整运行目录。GitHub自动生成的Source code压缩包用于开发。

本机运行目录：`F:/HearthwardDemo/v0.3.0/Windows`。历史候选以相应报告和公开发布页为准，本机旧打包目录可能已清理。

## 当前内容

- 山地石堡开场、夜袭撤离、自然四区、主线/支线内容、第二营地，以及现有战斗、生存、采集、建造、制作与存档系统。
- 弟弟跟随、委托与进度查询、续接确认、族人采集队；地图、任务地点指引及背包/仓储界面。
- 角色服装、武器与弓箭、救援动作、步态接触修正、石堡内外照明、自然地标、四区建筑、作物及营地设施升级表现。
- 落地、入水、出水、活跃火焰及局部风声；草/土/石/木/未知地面各3个脚步变体。当前26个声音事件、31份引用WAV。
- 主角与弟弟共享的石斧挂接和握姿修正，生产斧柄资产已保存并重开核验。[099/104整合记录](docs/qa/TASK-104/integration-20261010/REPORT.md)含实际持斧画面。

## 操作与验证范围

WASD移动，鼠标视角，Shift冲刺，空格跳跃，E交互；X弟弟跟随、Z等待；Tab背包、T交流、R仓储、B建造、K技能、J任务、M地图、F6存档管理、Esc返回/暂停。完整操作见随包README-DEMO.txt。

v0.3.0 Shipping Build/Cook/Stage/Archive通过；已实际验证CPU新游戏、背包、地图、手动保存，以及Vulkan新进程继续游戏和保存节点恢复。包内音频引用、本地模型/运行库及必要许可文件齐全。65项工具测试、PR CI通过；整合阶段的原生结果与截图见对应报告。

**完整游戏流程、设备声音混音、旧档兼容矩阵、第二台机器和完整性能验收仍未完成。** 既有语言理解质量与1% Low性能未达到原门槛；持斧小指较松、全部战斗/采集动作外观和部分建筑内室仍有限制。TASK-084—104未因版本发布自动标为Done，来源/Owner验收仍按任务记录管理。

## TASK-105 M0 本地工程候选（未发行）

`codex/TASK-105-engineering-optimization` 已整合一般/战役目录索引和战役计数优化，并修复 UE 5.8.2 JSON 字段键类型差异、OBJ 换行校验及旧版本测试夹具。不改玩法数值、存档、模型配置或 Content 资产。

12 表生产检查、新增工具 20 项、相关原生 37 项（含失败项修复复测）、Editor 编译及 Shipping 打包通过。正常窗口已检查新游戏、背包、地图、手动保存与重启继续，可见装备实例、耐久、负重和保存节点保持。5 项原生测试清理警告保留。完整首救路线、六场景性能、真人/二机尚未完成，不宣称帧率提升。

本地候选入口：`E:/HearthwardQA/TASK-105/m0-candidate-20261011/Windows/启动M0候选.cmd`。版本字符串仍为 0.3.0，用 BUILD-INFO 区分本地工作树候选；没有替换正式发布包。本轮源码按用户授权通过 [PR #70](https://github.com/XLingyyy/Hearthward/pull/70) 提交并整合；合并状态以该 PR 为准。

[验证报告](docs/qa/TASK-105/REPORT.md) · [交接](docs/handoffs/TASK-105.md) · [工程计划](docs/planning/TASK-105/ENGINEERING_PLAN.md) · [后续内容规划](docs/planning/TASK-105/CONTENT_ROADMAP.md)。M1 已在独立分支开发，M2–M3 保留提案。

## TASK-106 M1 源码整合（未发行）

`codex/TASK-106-m1-content` 增加旧渡口、倾倒运输车、风口路标、废弃炭棚、崖下药圃和林间猎迹；新增「渡口留下的东西」「没有带走的工具」两条支线，保留原 8 主线、15 旧支线、10 名救援人物和驻军分母。重做 side_06／08／09 的现场指引、完成反馈及营地陈列。

到现场按 E 查看线索或领取；运输车的木材、绳索、熟食分别受实际行装容量约束，取完不会刷新。两件剧情物以现有任务事实保护，带回营地陈列处按 E 交付，再从 J 登记完成。风口只揭示一个地点；猎迹指向当前附近真实动物，不直接发放肉。药圃走原有草药采集规则。

营地「出发与分工」路牌按 E 翻页，提供准备、外出、救援与返程、回营核对四种场景指引，复用手动任务卡和 X／Z 跟随等待。真实入库、救援到营、工具交付和合法用餐分别触发有限陈列；火边停留只使用空闲族人。新档与旧档均使用既有 Facts 和资源记录，不新增存档字段。

本轮已提交并推送，通过 [PR #71](https://github.com/XLingyyy/Hearthward/pull/71) 整合，合并状态以该 PR 为准。编译、定向测试与实机检查范围见 [TASK-106 报告](docs/qa/TASK-106/REPORT.md)。整片区域连续路线、弟弟／救援人物通行、昼夜近景与真人体验退出标准仍需验收；本分支不等于 v0.4 已发行。

## 本地开发

工程为 `Hearthward.uproject`，入口地图 `/Game/Hearthward/Bootstrap/L_Bootstrap`，目标UE 5.8.2。首次克隆运行 `git lfs pull` 取得二进制资产；本地AI准备方式见 `scripts/local_ai/prepare_bundle.py`。

已配置环境可运行根目录“启动测试版游戏.cmd”；该入口使用Development Editor和独立TestClient配置。C++修改后先构建：

```powershell
..\.venv\Scripts\python.exe -X utf8 scripts/ui/verify_input_client.py --build --label local
```

本机设置来自被忽略的 `.agent-local/environment.json`，可用 `HEARTHWARD_FACTORY_ROOT`、`HEARTHWARD_UE_ROOT`、`HEARTHWARD_PYTHON`覆盖。引擎生命周期统一使用GameFactory UEClient。

[仓库结构](docs/REPOSITORY_LAYOUT.md) · [构建与测试](docs/qa/BUILD_AND_TEST.md) · [工作流](WORKFLOW.md) · [测试端说明](TestClient/README.md) · [084—103执行记录](docs/planning/TASK-084-103/EXECUTION_STATUS.md)
