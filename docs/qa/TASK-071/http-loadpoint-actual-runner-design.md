# TASK071 实际HTTP请求跨LoadPoint QA草稿

状态：仅自身QA设计/脚本。未运行UE/build/model/HTTP/Git，未修改Source/Content/集成QA。根执行后才写实际结果。本脚本没有固定response、HTTP mock、模型替换或旧回调人工调用。

## 真实路径与观察前置

LocalAISubsystem.cpp230–255实际Submit记录玩家原话并取得内部CommandTicket；Tokenize与模型启动也是busy。仅is_busy不足以证明开始生成。本草稿等待公开is_busy=true、generation_calls=1、status="弟弟正在思考"、raw为空：Generate467–525先创建真实POST /v1/chat/completions，设置该status并ProcessRequest；ProcessRequest同步失败会Fail并清busy，无法通过该前置。脚本在观察到该状态的同一Slate回调调用实际LoadPoint，不等待固定延迟后猜测请求阶段。此证明UE真实HTTP已dispatch，不能单凭公开状态宣称服务端已经产生token。

LoadPoint429–439重新读池、校验和Restore；Restore351调用ResetForSnapshot，后者调用CancelPending：增Serial、CancelRequest、清CandidateId、bPending、raw/context与PendingSpeaker/Ticket。Generate回调482–486首先比较ExpectedSerial；存储AdvanceTimeline343–348同时改变epoch。ConfirmCandidate309–314核对真实ID、pending ticket、记忆revision及restore/paused状态。该测试使用公开接口观察，不打开私有Ticket/Request或新增测试setter。

## 最小三轮实际请求

1. 完整复用TASK068已运行公开夹具的Ground、实际Companion/camp/工作台、真实库存补给和baseline SavePoint；先记录实际baseline公开Phase/CommandId/Goal及任务量；前置按requested/delivered/acquired/carried全0且Goal无材料目标判断，不要求Idle。第一轮提交冻结公开C01，等待真实raw和实际可确认候选，保存其真实CandidateId，但不确认。实际LoadPoint后旧CandidateId必须STALE_CONFIRMATION，库存/源剩余/实例GUID和耐久/请求量/交付量保持baseline。
2. 第二轮提交公开C02，在上面的真实生成前置满足时Load同一baseline。立即检查epoch变化、busy/candidate/raw清除，观察3秒无旧结果发布，再实际SavePoint检查正常新SaveId与公开任务量仍0、库存不变。此3秒检查只是近端证据。
3. 第三轮再提交稳定C01，真实raw/候选通过且ID不同，旧CandidateId在新卡旁仍不能确认，新CandidateId可确认并实际执行完成。仓库wood精确+1、来源wood精确-1、Brother wood不增、玩家背包不变；通过正常SavePoint记录实际新SaveId及公开Completed/requested1/delivered1作为正控制。随后持续检查当前已完成状态直到旧HTTP dispatch后125秒，再SavePoint比较公开完整库存、来源数量与任务量无额外变化。该时域来自现Generate的120秒timeout/activity timeout加5秒；用Slate异步yield，期间不阻塞游戏线程。

第一轮旧candidate跨Load与第二轮真实pending HTTP跨Load分别覆盖；新Submit自己会废止前一candidate，不能把这种废止冒称Load效果。第三轮正控制排除“所有请求都坏了所以库存不变”的假通过。内部CommandTicket没有Python getter，不以新Request顶替它；既有Native ActualLoadPoint旧Ticket回归与此实际HTTP证据互补。

## 复用及输出隔离

`verify_http_loadpoint_pie.py`使用runpy读取现verify_model_matrix_pie.py；该模块顶层只建立尚未next的flow并注册Slate callback，实际setup文件写入在run生成器第一次next后。当前Python调用在游戏线程同步执行，没有Slate tick穿插。草稿取得run.__globals__，先把out替换成Saved/Task071/http-<label>-<pool>，立即注销原callback，再将QA selected_cases=[]/case_limit=1。之后由本脚本独立callback驱动原flow：只执行原world准备与baseline SavePoint，原60表达和20边界不运行。

原TASK068 results.json/cases.jsonl/boundaries.jsonl均不写；原模块顶层out.mkdir仅确保已有backend目录存在，没有覆盖文件。原夹具开始run时创建的两空jsonl会落在新Task071输出目录。未改冻结dataset文件或golden expected；没有源代码/private状态赋值。复用夹具现有公开prototype source_safe输入、显式敌人位置与材料补给，报告如实记录这些前置，不视为正常人类游玩胜利。

Engine脚本仅新增在自身QA；主机脚本`run_http_loadpoint_pie.py`沿现UEClient launch_editor/stop_editor串行持有一个PID，不读取key，不启动其他HTTP服务。bundle/backend/gpulayers与现实际Vulkan方法一致，Source控制模型4B/预算/线程/采样。输出launch/stop/results仅在新Task071目录，标准输出仅含白名单ok/stage/error/观察秒数，不打印模型stdout、APIkey或私人档。

根在当前模型试验PID完全退出后可执行：

```powershell
& G:/GameFactory/.venv/Scripts/python.exe -X utf8 G:/GameFactory/Hearthward/.agent-local/task067/docs/qa/TASK-071/run_http_loadpoint_pie.py --label actual-http-loadpoint
```

默认project是集成tree task051，pool为新随机UUID。默认900秒主机总deadline仅用于等待真实三轮/引擎启动；单轮仍使用现模型HTTP120秒限制。runpy前置及修正后完整脚本已做本地AST语法核对；子代理没有UE运行、C++编译或接口动态反射PASS。Root首跑结果见下面记录。

## 验收边界

旧请求被真实CancelRequest取消时，HTTP后端可能不再发出成功response。脚本没有强迫旧200回调到达，因此结果只能记“真实进行中HTTP跨Load取消/隔离、旧确认拒绝、无额外结算、新请求可用”。ExpectedSerial迟到成功分支是否实际触发没有公开计数器，保持forced_late_successful_response=NOTRUN。不得把125秒观察等同于成功迟到response已交付，也不能用伪造回复补这项证据。

菜单按钮/物理键鼠、正常标题Load流程、实际Slate旧控件callback、人类角色台词评价均NOTRUN。C01正控制若未产生匹配真实候选、HTTP阶段无法满足、实际Nav/Save前置失败，报告对应stage失败并停止，不填造候选或绕过条件。该脚本是071定向回调QA，不是068模型完整门或072性能测试。


## Root实际首跑与最小修正

Root首跑输出保留于`G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task071/http-actual-http-loadpoint-reboot-7c2d77fa-723c-495b-8f63-8702561881d4`，在fixture阶段读取SavePoint.World遇到protected cannot read，实际model请求为0；进程已关闭。它是Python访问夹具缺陷，未得到HTTP业务结果。

World在当前UE Python中受保护。初稿把GetPoints返回节点中的嵌套World视为公开可读，实际不成立。修正已删除全部World及NPCReceipts/NPCOperations/CommandActive读取；也不调用私有schema helper或打开生产getter。`http-loadpoint-public-api-only-fix.patch`提供精确增量，自己的verify脚本已同步。

save_now只读取公开save_id及Brother公开GetPhase/GetRequested/GetDelivered/GetAcquired/GetCarried。库存证据来自既有public describe_inventory/GetItemCount；包含双方背包的实例GUID与耐久、仓储数量、真实来源剩余。没有反射隐藏存档payload。report新增`npc_receipt_operation_checks=NOTRUN: no public Python API`，撤回此前规划的HTTP-runner回执/operation信用；原生Save13的receipt幂等证据保持独立，不计作本脚本执行结果。

修正AST通过，旧World/receipt/operation读取静态检查为0；host和三轮真实HTTP流程未改，修正后的实际运行仍NOTRUN。


## Root第二次实际夹具失败与语义前置修正

Root第二次运行使用pool前缀4399ff10，仍为0模型请求；失败为初稿的baseline public task is idle断言。StartNewProgress/正常wait指令没有承诺Phase=Idle，不能从枚举名称推定没有委托。该次结果和进程关闭事实由Root保留；不据此认定模型、Load或库存业务失败。

`http-loadpoint-baseline-semantic-precondition-fix.patch`先把公开GetPhase/GetCommandId/GetRequested/GetDelivered/GetAcquired/GetCarried及GetGoal的intent/item/quantity/mode/source写入setup.baseline_public_task，再断言四任务量为0且Goal是默认无材料目标。Goal字段已核对AgentContract.h6–18的BlueprintReadOnly公共属性，使用正常Python属性读取；没有protected/私有读取。Baseline Phase只记录，近端与125秒watch不比较其枚举值。第三轮真实确认执行后的Completed与delivered1仍是明确正控制。

自身verify脚本已更新，AST通过；修正后实际运行未执行。若真实公开Goal或任务量不满足前置，报告包含这些实测字段并停止，不清Command/Goal或强设Phase来凑条件。
