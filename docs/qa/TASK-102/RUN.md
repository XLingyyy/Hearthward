# TASK-102 稳定单场诊断入口

仅 Root 串行运行；已完成 [Vulkan单场](settled-dialogue-vulkan-20261007-01/summary.json)与[CPU独立单场](settled-dialogue-cpu-20261007-01/summary.json)：双后端1%Low FAIL、Vulkan语义错误／CPU HTTPtimeout与各自容量风险保留，六场／正常OS／Shipping未验。本子Agent未启动UE或模型。入口复用 TASK-072 已实测 standalone RemoteControl host，CSV parser、原统计公式和同日志模型区间分析来自本单 run_prologue.py。没有修改旧072、生产 Source、Content、Save 格式或模型锁。

```powershell
& 'G:/GameFactory/.venv/Scripts/python.exe' -X utf8 'G:/GameFactory/Hearthward/docs/qa/TASK-102/run_standalone_joint.py' --backend vulkan --run settled-dialogue-vulkan-20261007-01 --timeout 600
& 'G:/GameFactory/.venv/Scripts/python.exe' -X utf8 'G:/GameFactory/Hearthward/docs/qa/TASK-102/run_standalone_joint.py' --backend cpu --run settled-dialogue-cpu-20261007-01 --timeout 600
```

两个后端分别使用新 UE/模型生命周期及独立 `.agent-local/qa/TASK-102/<run>/profile`、UUID pool、临时 RC 端口和精确 session allowlist。必须等前次进程退出后运行下一条；同名输出已存在会拒绝覆盖。只读 Win32 查询自有 PID，Fatal/Critical 或进程提前退出记录 FAILED_STARTUP/FAILED_RUNTIME，保留原日志和部分文件；不会关闭 Nanite、调低画质或自动重复生成。

正常 Bootstrap public Screen.ExecuteAction('new') 后重取实际自然 World/Pawn/HUD/Screen/Loading/AI 引用，确认 HUD、Loading=false、自然世界启用、唯一真实兄弟和零历史模型活动。随后正常进入交流页，确认 CanCommunicate、Source 木材、未暂停；设置并逐项读回1080p目标下的 sg3、100%、TAA2、DX12、SM6、VSM、Nanite、无动态分辨率/VSync/限帧。Loading/页面/暂停/交流条件连续稳定10秒，全部发生在 capture 开始前。

使用 CsvProfile STARTFILE/START 建立唯一真实 CSV 文件后，只提交一次原072原话“采集1份木材送入营地仓库”。没有 ConfirmCandidate、移动人物、改进度、造材料或改时钟。每秒记录 Busy/Ready/GenerationCalls/Status 以及 Loading/页面/暂停/交流资格；至少等待请求终态且 capture 观察≥61秒，再 STOP 并等待末尾完整 metadata。解析全部数字帧且累计 FrameTime 必须≥60秒；坏数值/缺列/不完整 CSV 明确失败。目标渲染 CVar 在采样前后读回，actual systemresolution、D3D12/SM6/VSync off 来自完成 CSV metadata；屏幕截图像素不充当渲染分辨率。

输出 results.json、frame-report.json、CSV、runtime.log、local-ai.log、http-events.jsonl、model-progress.jsonl、resources.jsonl、RemoteControl.ini 和 stop.json。HTTP headers、模型进程完整命令、密钥不读取或输出；stdout仅输出安全状态摘要。资源为10秒低频 CIM UE/model RAM/pagefile及OS余量和GPU总显存/温度/功耗；GPU总量不能归因单一模型进程，缺每进程VRAM证据明确保留。

单条语义候选是否正确与帧指标分别登记。即使捕获完整，模型错误/无raw/HTTP失败保留失败；单条正确理解不代表087完整矩阵通过。模型区间由同一 UE log 的 generation→HTTP终态与 capture start/stop 交叉验证，整个采样包含冷启动/实际推理/HTTP/UE复核及后续真实UI刷新，不删除慢帧。公共 IsBusy 终态为1秒粒度观察，不能替代首次Paint、≤0.2秒反馈或暖p95；TTFT为NOT_RUN。所有结果仅为 normal_new_initial_scene_dialogue_fixed_view 注入输入诊断，不给正常OS操作、完整序章、Shipping或六场矩阵信用。

其余五个条件暂未实施：

|条件|缺少的真实前置|
|---|---|
|营地昼|已证实的安全营地 checkpoint、真实设施/族人/生产和实际 daylight 窗口|
|营地夜|同上，另需真实床/篝火的付费 Sleep/Wait receipt 进入夜晚；不直接写时钟|
|三人战斗|真实玩家/弟弟/敌人持续战斗区间；不造永生、假攻击或假伤害|
|路线流送|真实已解锁路线 checkpoint、实际移动和WorldPartition新cell加载证据；capture内不瞬移|
|营地管理|真实安全营地、设施/族人/生产及真实面板，MenuPause=false并记录生产变化|

这些是工程验证前置缺口，当前均 NOT_RUN/DEPENDENCY，不是统一等待 Owner 决策。旧071 Graybox营地和087矩阵移走战斗角色/临时floor不移用。根代理的9000/24000帧 OS boot 诊断独立保留，不能混入稳定场景。
