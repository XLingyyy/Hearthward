# TASK-072 Game RC FName显示大小写审阅

Root实际Game public references已通过，随后title比较因HTTP返回Title而失败，UE50708已退出，Submit/model0；保留原失败。源码ScreenWidget.h31 GetPage返回FName、137 Page=title；AgentContract.h9–10 Intent/Item为FName，12–13 QuantityMode/SourceRef为FString。

本机NameTypes.h33默认WITH_CASE_PRESERVING_NAME=WITH_EDITORONLY_DATA；629及IsEqual定义明确忽略大小写身份比较。Game可显示名字池内已有的Title拼写，生产Page==title比较仍成立。QA在序列化FName边界对四处GetPage与candidate Intent/Item做casefold，符合该契约；保留原始HTTP事件，不更改模型raw、原话或其它字段。QuantityMode/SourceRef是FString，保持精确比较；无Source/Schema改动或重Cook需要。

[官方FName说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/fname-in-unreal-engine)也明确名字比较忽略大小写。本审阅仅确认类型和QA调整依据；后续Game诊断结果仍以Root真实执行为准。子代理未编辑Root。
