# TASK-072｜验证游戏与模型联合性能及独立机器兼容

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿074，阶段D，P0；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-051、TASK-065、TASK-069、TASK-070、TASK-071。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

已有固定视角性能记录、CPU／Vulkan本地模型、Shipping Demo和安装器；它们不证明当前全地图和模型同时运行。目标是基于最终候选源码／资产的实际联合性能、冷暖模型及独立机器运行，最低配置由实测给出。

## 场景与测量

沿ACCEPTANCE锁定目标机、供电／温度、1080p原生极高和渲染设置；营地昼夜、序章、三人战斗、路线流送、营地管理分别与真实CPU和Vulkan模型请求同时运行。每场记录帧时分布、p99、1%Low、>50ms占比，标注加载排除窗口／采样时长／原始trace，不能只给均值。

模型冷启动ready与暖请求完整UE校验回复分开；记录排队、投影／tokenize、生成、HTTP、UI全过程及等待反馈。Vulkan10s／CPU30s暖p95、冷60／90s、反馈.2s和帧率门槛沿已批准值。统计显存／内存／页文件／可用RAM及OOM，默认16GPU层和并发1不为过测静默改变；单后端通过不掩盖另一后端失败。

## 兼容与诊断

从正常UEClient／现有打包脚本生成Win64 Shipping，独立第二台机器无UE／Python／API密钥，含所需runtime与GGUF，通过安装／启动／正常切片／保存继续／模型／断网、中文空格路径和非管理员可写存档目录。GPU后端不可用时验证已支持CPU回退和可用性反馈，不能把超时显示完成。

失败先用Timing／Memory Insights定位首个可复现瓶颈，分别处理同步加载、动画／AI、渲染、模型线程／显存竞争或打包遗漏；不据一条错误归因缓存／网络。性能修复保持玩法／模型能力，任何降低目标画质或更换模型的建议另给Owner审核。

## 出口

报告每场每后端结果、设备／驱动／锁版本、源码／cook资产、raw数据、最低／推荐配置建议及独立机器记录。性能优化后只重测影响场景与共享瓶颈相关场景；无证据不全仓重构。第二机器未提供或样本不足标未执行，不能用本机新目录冒充双机。正式公开发布不包含在本单。

## 建议施工范围

- `docs/qa/TASK-072/`
- `scripts/release/package_demo.py（仅可复现打包缺陷）`
- `Source/Hearthward/AI/（仅实测瓶颈文件）`
- `Source/Hearthward/（其余瓶颈须先精确登记）`
- `Config/（实际批准性能参数）`
- `docs/ENVIRONMENT.md`
- `README.md`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-051-input-traversal](../../contracts/CT-TASK-051-input-traversal.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)、[CT-002-companion-command](../../contracts/CT-002-companion-command.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
