# TASK071 显式两进程核心往返

测试入口仍为 Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket。无 phase flag 保留既有真实 LoadPoint/旧ticket拒绝/fresh ticket正控制路径。新增窗口只有 SaveTests.cpp 的该函数和 HAL/PlatformProcess.h include。

- write：独立测试池须为空；复用真实 Hero/Controller/Brother/source fixture 和公开 StartNewProgress。公开 Clock.Tick(7) 创建 A=7/W=7；TryAdd 创建真实 axe 实例、Brother wood2；Storage.Adjust 创建 shared wood6；PutPlayerMemory 写真实 agreement；RememberExchange 留正常原话；正常 Request/SubmitGoal 的collect接受后，以公开 SavePoint 保存。持有木材由正常接受逻辑产生 retained_adopted 事件和真实 acquisition/carry；没有测试setter或伪造command状态。
- write 从实际新节点保存 GUID/库存/Clock origin/NPC revision与事件GUID/会话时钟/原话摘要到 Saved/Task071/<pool-Digits>/manifest.json，只含本测试自己生成的状态。
- read：只 EnablePrototype 读取同测试池，跳过 StartNewProgress；校验 writer PID 与当前 PID 不同，按 manifest 的 SaveId 定位节点。先准备不同库存及无axe实例，随后连续两次真实 LoadPoint。
- 两次Load检查实例GUID、四处库存、Command/Campaign GUID和进度、A/W及origin派生日/分钟、真实记录原文/ID/活动命令事件/coverage/会话时钟/知识不增。最终公开 SavePoint 确认 operations/receipts 未增加、origin和knowledge revision保留。
- 显式phase使用固定World名称 Task071RestartFixture，保证两进程快照Map前置一致；无flag仍默认无名World。

已完成静态检查：公共声明和 UE JsonObject/AutomationTest 签名核对；增量patch在内存中按当前根逐hunk回放完全匹配；函数外（除PID include）与根一致；原无flag末段字节相同；没有 Store->State/Clock.Install/test setter。尚未编译或运行UE，不标记PASS。

根已提供 runner --pool UUID。选全新UUID并按以下顺序运行，write完全退出后才启动read；两个命令使用同一个UUID和精确同一filter，但不同label：

```powershell
& G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-053/run_native.py --filter Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket --label restart071-write --pool <fresh-UUID> --extra-arg=-Hearthward071RestartPhase=write
& G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-053/run_native.py --filter Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket --label restart071-read --pool <same-UUID> --extra-arg=-Hearthward071RestartPhase=read
```

范围限制：本核心用例只验证开发fixture中的真实公共存读路径；A/W同为7，生产设施跳时造成A/W分离未在本用例准备。后台生产批次、图箱Pending、双营地、成熟生态、自然地图实际PIE退出继续和HTTP/UI旧回调尚缺真实前置，均未宣称通过。旧个人档未写入、未提交；本实现未运行UE/build/Git/HTTP/model。
