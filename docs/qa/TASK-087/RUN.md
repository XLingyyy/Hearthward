# TASK-087 定向执行入口

原始集合来自 `docs/qa/TASK-068/cases.json`，保留其全部60条原文、分类、预期字段和30个执行用例。当前副本与来源对象相等；未精选表达或改变原阈值。

由主代理使用公开 `UEClient.runtime.launch_editor` 启动项目，并沿现有 `-ExecutePythonScript=` 参数运行：

```python
from engine_adapters.ue5 import UEClient
from uuid import uuid4

# 使用当前已配置项目的 client；CPU/Vulkan 串行，各用新的 run_id。
client.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation', extra_args=[
    '-HearthwardSaveTestPool=' + str(uuid4()),
    '-HearthwardAIBackend=cpu',
    '-Task087Run=baseline-cpu-20261007',
    '-Task087Revision=6fcf5c22e965f0f7409438f19bc7b09e96ffb058-dirty',
    '-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/TASK-087/verify_model_matrix_pie.py',
])
```

Vulkan 换成 `-HearthwardAIBackend=vulkan`，使用独立 run_id。GPU层沿运行时默认16；CPU实际为0。脚本使用真实 `SubmitPlayerText`、UE解析校验、确认和物品事务。输入3328、输出256、温度0、单并发、业务HTTP超时120秒和原理解/执行阈值均保持原值。脚本不启动独立模型服务。

沿原068实际launch使用已存在的`L_GrayboxValidation`诊断地图，避免正式地图中已存在伙伴/开场状态影响原夹具；`HearthwardSaveTestPool`每轮用新UUID隔离正式存档。主代理保留本轮准确源码差异记录，现有MCP/RuntimeInput端口继续由其配置的client统一分配。脚本显式建立测试世界前置、原型保存点、材料及位置夹具，因此结果属于真实模型＋诊断PIE，正常地图输入另行登记。使用未编译代码、普通正式存档池或另一模型运行参数时，不能作为本轮最终结果。

输出目录：`.agent-local/qa/TASK-087/<run_id>/<backend>/`。含逐条 `cases.jsonl`、20边界 `boundaries.jsonl`、完整 `results.json` 和可重算 `cases.csv`。完整60条另有独立 `performance_auxiliary.jsonl`，其辅助请求不进入原语言或执行分母。已存在目录会拒绝复用，保留中断失败。完整模型lock记录作为已锁指纹，额外记录模型尺寸/修改时间；没有疑点时不重算大文件SHA。

用于定位的最窄复测可加 `-Task087CaseIds=C01`，或逗号分隔多个原ID。该结果会标记 `diagnostic_subset=true`，完整门槛不会改用子集分母。最终需删除子集参数，CPU/Vulkan各完整60条，同一受测实现重跑。

历史TASK-079的单请求约50–95秒，因此60条每后端估计约50–95分钟，另加确认执行及边界耗时；这只是历史估算。本轮运行时按真实日志记录耗时，失败或超时保留，不能用历史成功替代。

原生过滤器：`Hearthward.Iteration.Task087.`，当前7项（生命周期3项＋`Hearthward.Iteration.Task087.Projection.`投影4项）；现有079 TaskContext/TaskContract及BoundedContextProjection兼容3项单列。实际RED/GREEN分别保存，联合Native 16项为13P3F，不能登记整轮GREEN。修复前Vulkan原60完整RED与Source.2定向12条FAIL也分别保存；Source.2最终Vulkan原60已实际32/60原始、24/30行为FAIL，并有独立60暖组件p95 PASS；最终CPU完整60实际33/60原始、25/30行为FAIL；真实IME仍NOT_RUN。

## 已启用的独立暖辅助

脚本默认`case_limit=60`且未传`Task087CaseIds`时，原60条结束后、原20边界之前固定执行一次原C01的`WARM-C01-01`，无需额外开关。subset诊断不添加辅助。该请求按既有公开夹具恢复输入上下文，只提交原文并读最终结果，不确认提案；失败保留，不重试，不进入原60理解、30行为或20边界分母。后端此时非ready则记录辅助失败，不另起一次冷生成冒充暖请求。

原`warm_component_latency`统计保留，新增`warm_component_latency_with_auxiliary`明确各自原暖数量、辅助数量及失败；下限60样本、nearest-rank p95、Vulkan≤10秒/CPU≤30秒保持。失败、空回复、非暖和缺耗时不会被当作有效暖样本筛掉后补采。CPU HTTP超时会停止当前runtime，下一条重启属于cold_restart；最终CPU实测C01 cold_first107.672秒完成、无cold_restart，后续59条确为暖请求；旧PIE单C01和Standalone不同场景超时分别保留。final-vulkan-20261007-02原暖59不足保持，固定唯一辅助后60暖p95=8.672秒组件窗口PASS；原32/60语言及24/30行为仍FAIL。GUI/UI paint及102联合性能仍NOT_RUN。逐轮事实与Owner待决策项见[MODEL_RETEST_DECISION.md](MODEL_RETEST_DECISION.md)。

最终`final-cpu-20261007-01`原始33/60、受限3/20、明确E2E33/40、行为25/30均未达语言门槛，边界20/20、白得0。原59暖不足保留，唯一预先声明辅助后60暖p95=22.65600000001723秒≤CPU30秒，组件窗口PASS；UIpaint及完整joint验收不由此代填。最终CPU/Vulkan JSON、CSV和独立aux均见REPORT，私有run根保存tracked.patch/untracked-files、launch、runtime、slots及隔离profile。两个完整后端已实测，无需为获得更多信用重复本轮固定矩阵；Owner契约决策仍PENDING。
