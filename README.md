# Hearthward（归火）

UE 5.8.2 单人第三人称生存冒险。TASK-076 整合版本：以 `main@ac6a302b` 的玩法为基线，接入 `codex/TASK-053-title-wheel@fa1828ed` 的新版 UI，并带入 TASK-075 文件整理。本轮整合分支为 `codex/TASK-076-ui-gameplay-integration`，经用户授权提交并更新 main。

入口：[项目状态](docs/PROJECT_STATE.md) · [本次整合与验证](docs/qa/TASK-076/REPORT.md) · [文件目录](docs/REPOSITORY_LAYOUT.md) · [分支关系](docs/planning/BRANCH_INTEGRATION.md) · [测试端说明](TestClient/README.md)。

## 启动当前版本

在已准备的本机环境双击[启动测试版游戏.cmd](启动测试版游戏.cmd)。该入口使用本仓库编译出的 Development Editor，从标题页进入游戏；存档、设置和日志保存在 `TestClient/`，不沿用其他工作树的测试目录。修改 C++ 后先重新构建：

```powershell
..\.venv\Scripts\python.exe -X utf8 scripts/ui/verify_input_client.py --build --label local
```

需要 UE 5.8.2、GameFactory Python 环境及本地模型资源。本机路径读取被忽略的 `.agent-local/environment.json`；可用 `HEARTHWARD_FACTORY_ROOT`、`HEARTHWARD_UE_ROOT`、`HEARTHWARD_PYTHON` 覆盖。首次克隆先执行 `git lfs pull` 取回二进制资产。UE 工程为根目录 `Hearthward.uproject`，入口地图为 `/Game/Hearthward/Bootstrap/L_Bootstrap`。

需要一次性独立测试档时运行 `python -X utf8 scripts/ui/launch_test_client.py --fresh-profile`。单独查看新版地图可使用[地图测试版.cmd](地图测试版.cmd)。开发测试端需要开发环境，既有 Windows Demo 安装包尚未同步本次源码；历史发行见[发行报告](docs/releases/demo-20260924/REPORT.md)。

## 本次保留和整合的功能

- 新版标题、加载、HUD、设置、背包、行装管理、技能、日志、建造和局部地图。背包支持拖放、装备穿卸、稀疏格子排列和四个快捷道具栏，排列与配置随原有存档恢复。
- main 的夜袭序章、兄弟同行与任务、探索、战斗、采集、营地建造与生产、种植养殖、维修、成长、生存救援及存读档逻辑。053—074 玩法批次已由 PR #60 合入基线；各单尚未完成的验收仍见原任务记录。
- M 默认打开新版局部地图；右上“世界地图”进入探索雾、已发现地点、任务指引、路标、驻军筛选和传送，继续遵从原玩法条件。可切回局部地图。
- 世界时钟继续驱动日夜、生产、刷新与旅行，HUD 显示统一日数和时刻；新档第 1 日 20:00 开始。保留加载输入恢复、伙伴攀越、倒地／全局失败和回档时间线保护。
- TASK-075 的素材归档、去重和目录索引。原始资源进入 `art_source/`，运行资源位于 `Resources/`，临时验证输出进入 `.agent-local/qa/`。

main 的正式 TASK-053 保持“基础通行”含义；UI 分支曾复用该编号，其本轮整合使用 TASK-076。原 UI 分支的大量历史 QA 材料保留原分支追溯，当前验证仅提交报告引用的证据。

## 玩法与操作

正常新游戏从夜袭开始：E 取护符，X 跟随／Z 等待，带弟弟沿后巷撤离，进入营地后按 J 查看目标。标题页“继续游戏”恢复最新节点；F6 或暂停菜单进入存档。旧营地档按已有迁移逻辑处理，原件及兼容备份保留。

| 功能 | 默认操作 |
|---|---|
| 移动／视角／冲刺／交互 | WASD／鼠标／Shift／E |
| 跳跃与攀越 | 空格跳跃；符合高度、体力和优先交互条件时 E 攀越 |
| 背包／技能／日志／地图 | Tab／K／J／M |
| 建造／伙伴交流／存读档 | B／T／F6 |
| 暂停、返回 | Esc 或 P；界面快捷键可再次关闭当前页 |
| 伙伴跟随／等待 | X／Z |
| 营地仓储 | 靠近存取设施按 R |
| 快捷道具 | 1 药品、2 食物、3 弓箭、4 投掷；再次按已选的药品／食物／投掷键使用 |
| 弓箭 | 选中弓箭栏且装备可用弓，按住左键拉弓、松开射箭 |
| 地图 | 滚轮缩放、方向键平移；世界地图右键设路标 |

按 Tab 拖动物品可调整同类格子或交换位置，拖到装备槽穿戴，拖出卸下；丢弃仍需 R。“行装管理”保留兄弟转交、仓储、具体装备实例维修及背包升级。物资转交仍检查距离、脱战和动作状态，维修与升级检查设施和材料。进食、用药各需要 3 个有效游戏秒，暂停冻结计时，取消不消费预留食物；伙伴自动进食保持原玩法。详情见[测试端说明](TestClient/README.md)。

靠近倒地弟弟 2 米内按 E 扶起；玩家倒地后 Esc 菜单可选择放弃，并需明确确认。普通菜单遵从暂停设置；交流页保持运行。设置更改需应用；显示模式和分辨率变更保留限时确认。

营地内可采集有限资源、入库、建造、制作、维修、休息和烹饪。靠近弟弟按 T 发出自然语言委托，检查候选任务卡后确认；任务卡与长期约定仍由现有规则及世界状态校验。时间、存档、伙伴和世界玩法契约见[有效设计](docs/design/CURRENT.md)及[当前状态](docs/PROJECT_STATE.md)。

## 制作源与动物演示

14 种动物配对骨骼、303 段动作、PBR 纹理和制作说明见[制作源索引](art_source/README.md)及[动物制作说明](art_source/TASK-051/制作说明.md)。独立实机入口为[启动动物实机演示.cmd](scripts/animals/启动动物实机演示.cmd)，采用临时档池；操作与验证边界见[使用说明](docs/qa/TASK-051/使用说明.md)。其他角色、武器及部件原始来源按目录索引保存，未统一重做资产。

## 验证与限制

本次构建、原生回归、独立游戏输入与界面检查的实际结果见 [TASK-076 报告](docs/qa/TASK-076/REPORT.md)。旧分支的 PASS 只证明原报告受测版本，不代表当前整合结果。UI 自动操作使用真实 UE Widget／Slate 事件，不能等同于完整真人通关。

已有待验收项继续保留：AI 完整理解矩阵未达门槛，CPU 推理曾超时；正式武器握姿、动作观感、角色美术和完整地图性能仍需验收；首版 30—60 分钟完整体验、第二机器及正式发行 RC 尚未闭合。本次未重做模型、玩法平衡或安装包。原证据与限制见[053—074 交接](docs/handoffs/TASK-053.md)和[工程性能诊断](docs/qa/TASK-072/game-development-runtime-review.md)。

模型使用项目锁定的 Qwen3.5 4B／llama.cpp；运行资源置于本地 `Runtime/LocalAI`，准备入口 `scripts/local_ai/prepare_bundle.py`。权重、密钥、用户存档、构建缓存不提交。多人开发遵循[AGENTS](AGENTS.md)和[WORKFLOW](WORKFLOW.md)，新任务先核对远端编号与分支，再从已核实基线创建独立分支。
