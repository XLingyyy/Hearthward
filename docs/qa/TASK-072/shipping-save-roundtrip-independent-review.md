# Shipping 手动保存／独立进程Continue独立审阅

实际writer与reader均通过。只读取两池results及HTTP日志，未启动UE、HTTP、模型或修改Root/世界状态。材料：[writer results](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-34f4495c-5604-4432-9d9a-b6fbee42a8ff/results.json)、[reader results](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-6b63a78e-b551-4e8f-9a0c-b2090091b496/results.json)。

writer pool `34f4495c-5604-4432-9d9a-b6fbee42a8ff`、PID45404，正常Bootstrap new进入自然地图后，公开SavePoint(Manual=true)实际HTTP200/ReturnValue=true；公开GetPoints从1节点增至2节点，新手动节点SaveId=`6518F48A4B9A1C3B9A00E8A5C753203F`，CampaignId=`8C62DF01461C1BBCCB374DA5029AB5DA`，Created={Ticks:639267533724500000}，为唯一最新节点；隔离pool.hws实际338704 bytes。FGuid字符串仅由实际A/B/C/D字段格式化。[writer HTTP日志](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-34f4495c-5604-4432-9d9a-b6fbee42a8ff/http-events.jsonl)

reader pool `6b63a78e-b551-4e8f-9a0c-b2090091b496`、PID46432，使用同一writer UserDir、独立输出及新进程。正常continue前，Bootstrap公开LoadPointIndex成功，GetPoints从真实pool返回相同唯一最新Manual SaveId、Created、Campaign；正常ExecuteAction(continue)实际成功进入新的自然地图引用。加载后IsNaturalWorldEnabled=true，GetCampaignId与writer相同，GetStatus精确返回“世界与知识已恢复；旧时间线请求已废止”，GetPoints仍有2节点。reader状态SHIPPING_SAVE_READ_PASS。[reader HTTP日志](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-6b63a78e-b551-4e8f-9a0c-b2090091b496/http-events.jsonl)

两次在new/continue前均公开验证Saved目录严格等于writer唯一UserDir/Saved；Shipping TestPool参数没有隔离信用。两进程gen0/server0，无模型/CSV、库存marker、定位或安全覆盖。恢复证据来自真实磁盘节点、正常Continue选点路径、当前Campaign及成功Restore状态。

当前没有公开CurrentSaveId getter。有限信用为最新手动节点被正常Continue选中并成功Restore，不扩展为完整库存、所有世界字段、真人键鼠或Shipping性能验收。

writer/read正常quit调用均HTTP200/ReturnValue=true，owned stop_ok=true；Root另行exactPID核对两进程均已不存在。host的stop成功无法区分自然退出与terminate，本审阅不归因退出方式。
