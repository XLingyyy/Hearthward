# UE 构建与关键回归入口

本机和已准备的 Windows runner 共用 `scripts/qa/run_engine_gate.py`，通过 GameFactory `UEClient` 执行 Editor 构建、隔离存档原生测试，可选 Shipping Build/Cook/Stage/Archive。

```powershell
..\.venv\Scripts\python.exe -X utf8 scripts/qa/run_engine_gate.py --output E:/HearthwardQA/TASK-109/gate-01
```

UI 相关检查加 `--render --tests <真实筛选器>`；加 `--package` 生成同次运行的 Shipping。输出目录必须是新目录，以保留之前失败证据。报告没有测试、测试未结束、任一失败或构建/Cook失败均返回非零。通过仍须查看单独保留的进程警告和错误诊断，不代表没有底层日志错误。

Shipping 的 staging 和 archive 都写入指定输出目录所在盘；UE Cook 缓存仍使用工程现有 `Saved/Cooked`。本机 G 盘空间有限，因此实际运行输出选 E 盘。

GitHub 的 `.github/workflows/ue-validation.yml` 仅支持手动运行，使用标签 `self-hosted / Windows / X64 / hearthward-ue582`。不让外部 PR 自动在本机执行。所需环境：`HEARTHWARD_PYTHON`、`HEARTHWARD_FACTORY_ROOT`、`HEARTHWARD_UE_ROOT`，已准备 UE 5.8.2、MSVC、对应 GameFactory 插件、LFS 内容及本地模型包。临时目录内生成产物，仅上传 JSON/日志诊断并保留 14 天，不上传模型或大型游戏包。上传动作固定为官方 v4.6.2 提交。

2026-10-11 实查仓库注册 runner 数量为 **0**。因此工作流文件已制作，远端 UE job 尚未接通或执行；本地执行结果独立记录，不把远端 repo-policy 通过计为 UE 自动化通过。runner 安装/注册后，才可验证该远端入口。

本轮最终选定 V2 AI 源码后，`integrated-v2-native` 的 Editor 构建和38项渲染原生回归通过：M0索引、M1–M3内容、存档、Agent契约/目录、固定情境持久化、休息设施与族人工作表现。族人工作夹具保留1条 PlayerInput 缺失警告。可移植报告为 `integrated-summary.json` / `integrated-native.json`；各项更早的定向结果见 `native-runs.json`，不重复相加。
