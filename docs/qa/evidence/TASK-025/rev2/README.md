# TASK-025 v2证据入口

最终结果入口：[RESULTS.md](RESULTS.md)，机器统计：[summary.json](summary.json)，提交绑定：[working-tree-binding.json](working-tree-binding.json)。

原版目录文件保留。本目录只记录增强版的研发、失败和验证，不用旧版PASS替代新结果。汇总由 `summarize.py` 从实际报告生成；未生成的报告为NOT_RUN。

## 复测

从GameFactory根目录，设置进程级PYTHONPATH后使用其虚拟环境：

```powershell
$env:PYTHONPATH='G:/GameFactory'
.venv/Scripts/python.exe -X utf8 Hearthward/docs/qa/evidence/TASK-025/rev2/run_evaluation.py build
.venv/Scripts/python.exe -X utf8 Hearthward/docs/qa/evidence/TASK-025/rev2/run_evaluation.py native
```

同一runner支持dev、exposed-heldout、heldout、timeout、regression、workshop、faults、lifecycle、interactions、playercraft、playerrepair、cpu、physical、physicalb。模型运行串行，避免两份4B和两个UE进程争用8GB显存。每次使用独立UUID存档池，正常用户档不参与测试；物理模式只准备/观察夹具，提交与确认必须实际输入。

编译与生命周期通过UEClient公开API。非物理模式加unattended，避免启动提示阻挡脚本。baseline使用独立detached worktree，代码保持d02b5fe，运行库/权重为同一部署，42个共同输入使用同一初始物资与位置；B作为新增能力单列。基线保留原自动执行策略，新版按批准策略确认。

## 证据层次和指标

- `native-index.json`：项目具名原生测试；完整UE启动诊断见native-results，不把既有Condition failed隐藏。
- `dev/heldout-results.json`：60开发+60当前留出；首轮留出失败及7条定向回归分开保留，见dataset-splits.md。保留每轮原始JSON、上下文、规范意图、原因、token和实际库存/源消耗/耐久。
- `regression-results.json`：原025的15个输入种子，确认策略与撤销容量按v2批准预期调整，检查真实执行和跨PIE磁盘恢复。
- `faults/workshop/lifecycle/interactions-results.json`：三轮关键状态边界、真实子进程故障与多轮/陈旧UI操作；脚本PIE不冒充鼠标实键。
- `playercraft/playerrepair-results.json`：保留023/024玩家原操作回归。
- `physical/physicalb-results.json`：computer-use实键轨迹。离屏CaptureUI图只作结构辅助，实际窗口截图另存。
- 每份manifest记录运行开始的SHA、源码工作树差异、测试集/脚本绑定、锁定模型和运行配置、GPU样本。新构建后才可启动评测。

原始模型理解率与最终成功率分列；模型误解被UE阻断或只读归类修正，仍记模型失败。受支持完整目标必须实际达成，条件目标单列；“没有提前写世界”只算安全，不能冒充任务成功。所有预期库存/回忆用例均进入可追溯答复的分母，即使模型误分类也不会被排除。库存语料询问木材，核对UE答复的准确数量短语与允许观察；回忆核对原话及ID。确认API反馈与生成耗时分开统计。

暖机P95排除每次进程首条冷请求。GPU峰值为整块设备使用量，帧时间为PIE Slate回调；CPU独立运行。Shipping、第二机器和独立人物自然度审查均NOT_RUN。

## 场景映射

| 要求 | 对应证据/检查 |
|---|---|
| A01–A05 | dev/heldout的采集、未确认、否定查询、补数量、未知地点；physical任务卡及实际交付 |
| A06–A09 | interactions插入闲聊/查询与改物品；dev/heldout追加量和多目标 |
| A10–A11 | Native CapabilitiesAndLimits；dev/heldout异常数量；目录直接驱动Schema及手动选项 |
| A12 | 最终已启用B，未注册制作/维修阶段不适用；无工作台/无唯一自有装备的不可用边界另测 |
| A13–A18 | Native旧档/记忆/累计费用；regression采集限制；workshop规则确认、版本固定、禁止材料和预算；physicalb长期规则卡 |
| A19–A22 | dev/heldout虚报与别名；regression离营3→8仍知3、回营更新；interactions无观察不冒充0 |
| A23–A26 | Native128次新增撤销与容量；regression编辑/撤销/回档；interactions陈旧记忆修订 |
| A27–A29 | 真实效果后事件；faults远距确认拒绝、已接受任务离开30米继续 |
| A30–A34 | faults耗尽、返程不可达、多趟、旧物；interactions他人入库不计进度 |
| A35–A36 | Native EffectReceiptsAndProgress及仓储原生测试：同操作同载荷一次效果，冲突载荷拒绝 |
| A37–A40 | lifecycle取消、interactions关对话和旧记忆按钮、workshop无效替换保留旧任务、faults跨epoch旧卡拒绝、regression旧回复作废 |
| A41–A44 | faults部分交付读档和重入保存；workshop制作维修回档；Native真实旧文件迁移和新格式校验；regression新周目 |
| A45–A46 | Native严格解析、lifecycle真实token超限/3次崩溃/显式重试和手动操作；timeout三次真实120秒静默超时、显式恢复；对应第二轮版本，第三轮未改生命周期代码，见delta-validation.md |
| A47–A50 | faults暂停确认与恢复；interactions三次恶意记忆、陈旧快捷操作；dev/heldout原始台词与实际任务卡/效果分开 |
| B01–B02 | dev/heldout制作、批数/成品单位；workshop授权材料真实取用；physicalb |
| B03–B04 | workshop到站前材料消失、NPC自有位置/背包；玩家两套回归 |
| B05–B06 | dev/heldout玩家装备拒绝、自有维修；workshop同类多件拒绝、重复确认和回档；physicalb |

矩阵说明覆盖关联，不将尚未生成报告的项标PASS。人物自然度需要独立审查；初始资料以角色身份为准，但4B仍可能在拒绝台词中暴露技术措辞。残余边界见 [limitations.md](limitations.md)，开发失败全部见 [evaluation-failures.md](evaluation-failures.md)。
