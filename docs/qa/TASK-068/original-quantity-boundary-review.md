# TASK068 原话数量核对增量

根真实U03业务RED：原话中文33、raw collect32、unresolved[]，candidate=true。此次增量基于根最新Schema B及NPCAgentTests箭矢[arrows]断言；仅四个已批准源文件，没有UE/build/Git/model运行。

- Contract.h/.cpp增加真实业务函数OriginalQuantityMatches，StageCandidate调用、Native同函数测试。只核对已选Goal的数量槽，不分配Intent/Item/Source/Mode，也不回写模型Quantity。
- 与现数字regex并行保留原缺量工作草稿行为。完整ASCII数字token用规范字符串等值核对（仅去前导0，避免整数溢出或子串误匹配），完整中文token以现0–99格式化语义比较，两→二/〇→零；百千万完整token不会截出小数值后缀，无大数parser。
- craft只认批；其余认现单位并要求数量绑定ItemText/实际ID相邻词、明确数量声明或现补充数量。消耗/最多/至多/不超过子句不能充当目标。多个有效目标数量、无匹配目标或数值不等按unresolved拒绝候选。
- 保留真实GoalText物品×N单位；缺量原话+补充三份正控制。显式UI AdjustCandidate路径不改。原Parse/Validate/Schema/Registry/Describe、8字段和所有预算保持原文本。

Native新增Hearthward.NPCAgent.OriginalQuantityBoundary含20条有意义边界/正控制：中文与ASCII33→32、132→32、十二→2、完整大数后缀、合法32、两及物品alias、结构化GoalText、只有消耗量、正常数量补充、数量声明、多量冲突、三批箭/三份或五份材料、消费量不能改批数、缺批/按支及结构化craft。

静态Python独立重建同一有限token核对，20条预期均符合；这是静态方案检查，未作为C++/Native PASS。公开SDK Regex.h已核对GetCaptureGroup/GetMatchBeginning/GetMatchEnding，UnrealString.h.inl已核对FString TrimStart/TrimEnd/RightChopInline接口。最窄根验收：编译；Native OriginalQuantityBoundary+CapabilitiesAndLimits；原固定10实际模型，重点U03 raw仍FAIL且candidate必须false、E2E安全通过。其他semantic raw失败仍保留，不宣称10或60门槛完成。

当前已知边界：有限中文写法仅规范0–99，其他写法请求澄清；无法可靠绑定的语序不猜选目标。现Stage数字槽支持的明确正控制已保留，不扩写意图语法/关键词分类器。不会将guard阻挡算raw正确。
