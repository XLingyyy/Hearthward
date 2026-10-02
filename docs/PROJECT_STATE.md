# Hearthward 项目状态

核对日期：2026-10-02。远端主干基线为`origin/main@4db5789184fe38e041d62a1e68c8517338ea0b01`，已含043—050、PR #55的UI／加载／场景修复、PR #57的051操作基线和PR #56的动物／更新兼容整合。各原始测试仍绑定其报告源码，不据main合并推定重跑通过。

当前分支`codex/TASK-052-time-integration`完成[052工程方案](planning/TASK-052/PLAN.md)及任务登记，运行代码、schema9迁移和共享契约已获Owner确认，正在施工。canonical052对应原稿054；[本单交接](handoffs/TASK-052.md)记录范围及证据。固定录音按Owner要求继续暂缓。动物归档沿用051标签；052保留该资料，不覆盖此前canonical051的操作实现。

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

[AGENTS](../AGENTS.md)和[WORKFLOW](../WORKFLOW.md)继续约束任务范围、LFS锁、真实验证和人工合并。当前052权限见其任务JSON，候选施工范围见052工程方案；动物同步授权保留于TASK-051 JSON。Agent没有自批、合并、发布或修改远端保护的授权。真实存档、本机密钥、权重、构建二进制和生成缓存保留本地。
