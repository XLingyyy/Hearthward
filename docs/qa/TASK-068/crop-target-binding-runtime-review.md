# TASK-068 唯一自然作物目标绑定实际 RED → GREEN

2026-10-05，Root，UE 5.8.2 / Windows 11 / D3D12，HEAD `67fb0784ca8c6d488173e587e7f95c4be0d9092a` 加本地未提交施工。实际 advanced pool `ae971106-7078-491c-9ee5-7ffa8c2a2a96` 的 ADV-N01 已证明 fixture 与原始模型结果正确，唯一已种且未浇水的作物仍返回 TARGET_REQUIRED、没有候选。

根因是 StageCandidate 中 deposit_feed 的无括号 for/if，随后 else 按 C++ 最近未匹配 if 绑定至单个 pen 的距离条件，导致 water/fertilize/harvest 没有进入 crop 枚举。先仅在既有 WarehouseMaterialsPresentation 添加公开 SetStructuredGoal 的唯一水作物阳性和饲料 pen 对照；实际 Native 恰好1个错误：water 不能形成候选。显式 GUID 的真实 Preview、feed 自动绑定、未确认 Nature/库存无变化均通过。

随后仅给 deposit_feed 与 crop 两分支增加4行 braces，保持枚举条件、距离、未知/多目标处理、协议和权限。Editor Development 实际编译 PASS；同一 render Native 该项 GREEN，0错误、1个原有 EnhancedInput fixture 警告。同行055轻/重均PASS且0警告；整组3/3。

记录：`crop-target-native-red.json`、`crop-target-native-green.json`、`crop-heavy-test-only-native-red-evidence.json` 与对应 Saved/Task053/crop-target-heavy055-*。补丁与第二轮真实模型失败均保留。该 Native 证明公开候选绑定和未确认无副作用，没有提升模型 raw 理解率。真实 ADV-N01 与修订 fixture 的 ADV-N02 均已取得独立实际 PASS，水作物状态和散石采集返营入库正确，见[真实模型补充结果](advanced-public-api-successful-runtime-review.md)。
