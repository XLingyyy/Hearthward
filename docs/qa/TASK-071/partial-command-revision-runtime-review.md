# 部分领料命令版本的实际 LoadPoint RED

2026-10-04，实际 EditorDevelopment 构建通过，`Saved/Task053/partial-command-revision071-red/index.json`：1 case Fail，8 Error，0 Warning。

首笔真实材料 Transfer 提交后，公开 OnTransferred 的测试监听器正常 Reserve 其他材料，制造实际 partial take；非私有状态注入。两次真实 Request/Discard 得到 r2。

- live 正控制：同一 r2 command，first=stone，解预留后 Resume→Completed，真实产物和材料去重断言通过。
- 实际 SavePoint→LoadPoint：保持相同 CommandId 和真实库存分割、取料回执；解预留后 Resume→WaitingAtCamp，实际 reason=SETTLEMENT_FAILED，产物0；恢复将 Active.Revision 固定1，与保存的r2领取payload冲突。
- Capture 当前只保存 CommandId/Receipts，未保存 Active.Revision。版本跨加载丢失已由实际同场景的live正控制与Load回归确认。
- 已登记精确生产Save窗口，生产修复与GREEN尚未执行。不能忽略结算payload或用任意弟弟背包库存抵消仓库授权成本。

## 实际修复 GREEN

`Saved/Task053/revision071-blade055-green-no-once068-red/index.json` 的全部Save用例通过（合并总运行另有两项No/Once预期RED，不能称总运行成功）。独立partial案例Success、0 Error/Warning，live及真实Load后的actual revision=3、first=stone均Completed；正常首Request因恢复计数器推进而本轮为r3。原receipt/CommandId保持，真实领料与craft产物各一次。

真实无回执r>1命令经过SavePoint→LoadPoint→SavePoint保持revision，下一次公开Request递增；标准HWS9 writer实际省略CommandRevision标签的档案经现ReadBytes接受，无回执保留既有r1缺省、含实际旧take回执准确推导r3。显式不一致、混合版本、负数、零、前导零、尾垃圾、overflow负夹具均拒档；未改Schema9/Header、旧epoch、材料来源或Settle的payload精确校验。公开诊断原因已在 no-once068-green-suffix-red-diagnosis071 的实际 Save 回归通过。

公开诊断后续：`Saved/Task053/no-once068-green-suffix-red-diagnosis071/index.json` 中 ActualLoadPointPreservesPartialMaterialSettlementRevision 为 Success、0错误／警告；显式命令／回执版本冲突的 Diagnose 输出原因断言通过。该次全运行另有两条材料尾部预期RED，不称整体PASS。
