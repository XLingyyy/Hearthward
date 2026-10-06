# Hearthward 项目状态

2026-10-06 TASK-083：当前基线 `main@af08e1ab` 已通过 PR #61 整合 TASK-082 地图、传送与简约界面，并保留 TASK-078—081。用户授权整理 main 并发布 `0.2.0-preview.20261006.2`；当前发行和独立程序验证见[发行报告](releases/demo-20261006-2/REPORT.md)。以下 .1 包及旧提交记录为历史交付。

2026-10-06 最新源码集成：用户明确授权 TASK-078—081 提交并推送 main，集成基线为 `ca21120cef4421d039c44248a0cfa5043c248feb`。包含弟弟委托奔跑与停工反馈、自然语言查询和续接、任务地点标点、右侧对话面板及族人采集队。最终 Editor 构建和本轮定向验证见 [TASK-081 报告](qa/TASK-081/REPORT.md)，各项历史测试仍绑定各自报告的受测实现。本次仅提交源码与必要证据，现有 Release 不变，任务保持 Active 等待人工体验验收。

核对日期：2026-10-06。集成前已核对远端 `origin/main@53a1efd0ddf169a36265930aea63b58cabfaf061`，包含新版UI与既有玩法整合。用户明确授权将TASK-077最新成果提交推送到main；本次集成在该基线上加入山地石堡开场、Marble裁分立面、相应测试与Windows Demo发行脚本。没有改动主地图文件或存档格式。

当前本地便携Demo为 `0.2.0-preview.20261006.1`，约4.88GB，目录 `F:/HearthwardDemo/20261006`。Shipping构建和打包通过；相关原生4/4、PIE撤离路线16/16通过，独立发布包实际完成新游戏、背包、地图、跟随和保存后重启恢复。详见[发行记录](releases/demo-20261006/REPORT.md)及[TASK-077交接](handoffs/TASK-077.md)。发布包不提交Git，已公开为[GitHub预览Release](https://github.com/XLingyyy/Hearthward/releases/tag/v0.2.0-preview.20261006.1)，三个分片及合并脚本均已上传。主堡内室、精细建筑美术与完整夜袭演出仍待完成，TASK-077保持Active。

TASK-076新版UI和玩法的整合证据见[整合报告](qa/TASK-076/REPORT.md)。以下各历史任务的测试继续绑定原报告源码，不据本次集成推定重跑通过。

052已由Owner通过PR #58合入main，包含批准的[工程方案](planning/TASK-052/PLAN.md)、schema9迁移和共享契约。Editor Development、相关原生34/34、渲染PIE82/82、工具33/33仍绑定受测源码`0bc1c3ee5d09cfca3f675cf657b6981f874f9138`，见[052报告](qa/TASK-052/REPORT.md)；任务元数据保持Active，不代填Owner体验／最终验收。canonical052对应原稿054；[交接](handoffs/TASK-052.md)保留本单证据。固定录音继续暂缓，动物051标签不改。

2026-10-03，Owner批准[053—074整批设计](planning/TASK-053-074/REVIEW.md)并授权施工。22项对应原稿055—076，设计均Approved；053—064、066—072及074已激活工程施工或定向调查；065／073真人验收尚未执行。原施工根目录为`.agent-local/task051`、分支`codex/TASK-053-traversal`；该目录作为已有工作树保留，新任务从核实后的main另开分支。本批实施提交为`aa174675e9bf2f3de797fde7cb6795c1152a9d1b`，已于2026-10-05经PR #60合入main，尚未正式发行；Owner／Reviewer均为XLingyyy，无Issue。当前证据见[053交接](handoffs/TASK-053.md)，历史设计核查仍在[设计REPORT](qa/TASK-053-074/REPORT.md)。

2026-10-05施工接续：055石斧重击的时序与旧扇形误伤已真实Native RED→GREEN，重击三例及轻击回归均0错误0警告；唯一自然作物目标绑定也已RED→GREEN。068真实模型补充浇水、新采散石返营和普通钓鱼均分别取得实际成功；散石仅修订明确隔离的QA营地代理位置，生产导航不改。071首次交流Slate崩溃修复后的公共卡片/菜单跨Load96/96通过，新增真实OS键鼠读档确认/取消路线通过；069交流深底和真实中文IME组合、候选、Enter/Escape优先级局部验证通过。核心/成熟批次/图箱Pending/生态/双营地独立往返、Save13/13与真实HTTP跨读档保留既有定向证据。

069正常窗口检查已修正标题／背包／暂停页脚、默认与150%背包可读性、Save行重叠及150%设置值消失；设置已实际恢复100%。34个任务目标在正常／真实受伤倒地Widget中量字通过，最多六行，正文24及时间／位置／生存按钮无交叠；原始证据见[界面报告](qa/TASK-069/normal-ui-readability-runtime-review.md)。正常Settings Apply后Continue的加载输入旧快照已真实RED→GREEN，相同实际OS路径Escape／Tab及自然Quit通过；见[加载输入报告](qa/TASK-069/loading-page-input-runtime-review.md)。默认地图探索文字17→24的实际Widget测宽高及进度条边界通过。

068完整理解门槛仍失败：目标绑定修复前CPU60 raw34/60、歧义1/20，先前Vulkan60 raw36/60，两个全矩阵版本不同，最新补充成功不抵扣门槛；字段顺序失败实验已撤回。055完整皮肤重建通过，单个缩径/手指组合穿小指已拒绝，正式握姿/动作观感和许可未验。070两件PBR样品与三组角色/营地/地形概念图已准备，Owner风格仍待审阅。正常Bootstrap新游戏已由实际键鼠进入夜袭，手动保存后的独立重启继续恢复可见任务／伙伴／库存，确认退出自然结束；只覆盖开局局部路线。九页完整流程、真人体验、联合性能/第二机器及Shipping发行仍未通过。 072正常Standalone公共引用接入已通过，见[接入报告](qa/TASK-072/standalone-reference-runtime-review.md)。 已有正常初始场景CPU/Vulkan各一请求与完整CSV，1%Low均未达60、CPUcompletion120秒超时、Vulkan候选正确；可用内存低于1GB的区间已实测，见[联合诊断](qa/TASK-072/normal-new-cpu-vulkan-runtime-review.md)。 CPU原生日志已定位提示词耗时；独立Game Development构建/Cook/归档通过，正常成品Vulkan候选及该场帧分布通过（1%Low61.44），CPU仍超时且1%Low58.17未过，见[成品诊断](qa/TASK-072/game-development-runtime-review.md)。独立 Shipping 工程 Build/Cook/Stage/Archive 及本机正常新游戏已通过，独立 UserDir 内手动保存后重启正常继续也已通过（同手动节点与进度 ID，完整库存核对未验），见[Shipping 工程诊断](qa/TASK-072/shipping-diagnostic-runtime-review.md)。未冻结正式RC。见[正常开局／继续／退出局部证据](qa/TASK-069/normal-game-os-runtime-review.md)。证据见[当前交接](handoffs/TASK-053.md)。

## 分支与文件入口

新版UI来源 `codex/TASK-053-title-wheel@fa1828ed`，TASK-076已整合，经用户授权更新main。默认使用新版局部地图，通过“世界地图”保留探索、任务定位、路标和传送。双方曾复用TASK-053；main保留通行任务，本轮整合使用TASK-076。详见[分支清单](planning/BRANCH_INTEGRATION.md)、[目录说明](REPOSITORY_LAYOUT.md)和[QA索引](qa/README.md)。

## 本次同步成果

`codex/project-progress-20261002`的同步成果已由PR #56合入main，正式Release尚未更新。原动物分支及其本地工作按原交接保留，052不清理或覆盖其他检出。

- [TASK-051](tasks/TASK-051.md)：14种动物、14套配对骨骼模型与Skeleton、303段动画、物种自然活动、主角／弟弟接近逃离、受击与致命倒地／沉降、死亡保持及严格边界的独立观察地图。基础逃离630厘米/秒，比玩家基础疾跑600高5%，随疾跑技能保持同倍率。
- [建模与动作制作说明](../art_source/TASK-051/制作说明.md)：归档当前R3 Blender、SK、FBX动作、PBR纹理、连续预览、14份指导、复核数据和制作工具源码；补齐15个此前未入库的角色、武器／护具FBX与生成清单。
- [更新与存档兼容](qa/update-compatibility-20260930.md)：旧营地迁移、新版进度隔离保存、冲突逐项预览与确认、原件备份、未来／未知损坏档保留及GitHub正式版更新提示。
- 结合远端PR #55，保留设置／主音量、加载过渡、场景表现和击晕提示，并接入兼容对话框、版本检查和动物致命动作。

当前归档与整合验证见[同步报告](qa/project-progress-20261002/REPORT.md)。动物原实现报告的199/199实机检查、6/6定向回归与原Source／DLL／资产指纹继续位于[动物报告](qa/TASK-051/REPORT.md)；这些历史结果不自动等同于整合提交通过。

## 运行与玩法入口

普通游戏从`L_Bootstrap`启动，正常新游戏进入夜袭序章。按E取护符、X跟随、Z等待，带弟弟撤离到营后按J查看目标。自然营地内有采集、伙伴委托、仓储、设施制作、建造、维修、休息、烹饪、地图、任务和存档入口。完整操作、模型准备与已知限制以[README](../README.md)为准。

动物观察地图独立于主自然地图，双击`scripts/animals/启动动物实机演示.cmd`启动。F5切换14种近景，F1回角色，F2弟弟接近／等待，F3致命伤，F4重置，Esc退出。使用临时档池；[使用说明](qa/TASK-051/使用说明.md)列明所需UE／GameFactory环境。克隆后先取回Git LFS资产。

## 验证与验收边界

各任务的原始UE构建、原生测试、PIE和截图继续绑定其报告中的源码SHA或Source／DLL／资产指纹，不据合并提交推定重新通过。同步PR的独立整合验证绑定同步报告列出的完整实现提交。

Windows Demo 0.1.0受测源码仍为`8b54550b5f9d7d01c9e9e0f7444826090667f3f5`，原发行基线为`main@68e68d8`，见[发行报告](releases/demo-20260924/REPORT.md)。桌面更新兼容预览版的Shipping检查属于其报告的历史本机构建。主干归档不等同于更新GitHub Release或现有玩家安装包。

TASK-026的4032米World Partition自然世界与营地已经可运行；完整地图路线、流送、性能和Owner视觉验收仍以[地图交接](handoffs/TASK-026.md)记录为准。TASK-048、049、050各模块的Owner实玩验收和首版30—60分钟节奏仍未全部闭合。TASK-051保留Active，等待Owner检查动作观感与速度；不填写虚构Reviewer。

## 设计、权限与后续

有效设计为[CURRENT](design/CURRENT.md)指向的GDD v0.3及已批准增量；未定规则继续见[OPEN_QUESTIONS](design/OPEN_QUESTIONS.md)。任务映射和早期AI标签按[TASK-041账本](planning/TASK-041-baseline-ledger.md)追溯。TASK-004源归档不等同于其余模型全部适配或整单验收。

[AGENTS](../AGENTS.md)和[WORKFLOW](../WORKFLOW.md)继续约束任务范围、LFS锁、真实验证和人工合并。052及动物施工授权保留在各原任务；053—074整批设计及施工已获Owner批准；激活任务按JSON中的准确源码路径施工，尚未登记的资产和共享文件先补精确范围。TASK-076用户已明确授权提交、推送和更新main；没有自批评审、正式发行或修改远端保护的授权。真实存档、本机密钥、权重、构建二进制和生成缓存保留本地。
