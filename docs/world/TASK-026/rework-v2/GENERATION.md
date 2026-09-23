# TASK-026 增量更新入口

使用 UE 5.8.2，编辑器打开 Rebuild/L_HearthwardWilds。原始全量生成入口仅用于首次创建；既有地图由显式批次清单更新。

## 归属与身份

ownership.json 使用 FGuid.ToString 的真实 GUID，cell 为完整 252m 批次。Generated 可由计划更新；Authored 拒绝被生成更新命中；Reserve.json 是数据约束。旧 str(Guid) 产生内存地址，已登记为无效并保留诊断记录。

## 操作

在 Saved/Task026/ReworkV2/request.json 写入请求，再在编辑器 Cmd 执行：

```text
py "G:/GameFactory/Hearthward/scripts/world/TASK-026/rework_batches.py"
```

- snapshot：action、output；输出完整批次、实例 ID、变换、材质、碰撞及非批次 Actor 清单。
- plan：另加 replacements、inputs。replacements 将明确 GUID 映射为完整实例列表；inputs 列出本次全部源文件与生成代码。output 必须为新的证据路径。
- apply：另加 plan。核对输入、计划、保护对象和批次状态，核验 30 分钟内远端锁快照，备份到 Saved 后只保存计划内脏包。超出计划即停止。
- promote_authored：plan 的可选 GUID 列表，只允许将显式 Generated 批次锁定为 Authored。

相同输入重放允许原生四元数 1e-6 数值误差；实测归一化偏差最大 3e-8，不能据此重复重建。其他对象的语义比较保持精确。

## 当前验证

R1-density 只把 grass_-4_-3 从 43 减为 22；stump_-4_-3 作为 Authored 保持 ID、变换、材质不变。R1-density-repeat 第二次相同输入无包保存，全部语义与第一次一致。原生保存曾因本机只读位失败；已在核验本人的远端锁后修复目标文件写入位，未强制解锁。

地形、水段和材质的进一步更新仍须各自明确包清单与依赖，不通过批次工具修改它们。
