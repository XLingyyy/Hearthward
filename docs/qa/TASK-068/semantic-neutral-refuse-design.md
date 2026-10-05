# TASK068 中性拒绝完整JSON单候选

状态：只写QA候选与静态证据，Source只读；未运行UE/build/model/HTTP/Git。

根已实际取得 fullJSON10：raw7/10，七个清晰表达的raw全部正确；三个非法/缺量表达的raw仍失败，现有UE数量/约束守卫拒绝了全部错误任务卡。actual input 3016–3146、full_relevant、dropped=[]。该候选只改善模型可见分支示范，不把守卫拒卡计作raw正确。

`semantic-neutral-refuse-candidate.patch`相对根当前System416–425，仅把第二个正例的末尾分号移到新行并加一个完整拒绝例（+2/-1）。原两个完整正例、clarify缺量规则、合法目标/消耗限制分离、最终运输终点、全部语义规则逐字保留。没有改Schema、Parse、Validate、目录、projection、知识路径、messages构造、缓存、模型或采样。

唯一新增示例：

```json
{"intent":"refuse","item":"none","quantity":0,"mode":"none","source":"none","limits":[],"unresolved":[],"npc_line":"哥，负批数无效；请重新明确合法的正整数批数，我再为你准备任务卡。"}
```

独立输入为“制作负六批烤肉”。原本负六不能变成六，拒绝输出使用既有中性槽，不产生craft目标/limits/待执行承诺，也不声称完成。台词32字，符合当前普通台词30–60字规则。Items与craftingRecipes都存在真实`roast`/烤肉，配方outputs roast1、facility campfire，因此拒绝展示的唯一实质非法条件是负批数。该输入不等于冻结60任一表达；冻结表达没有“负六”或ASCII“-6”。没有case-id条件、关键词路由、后处理raw或冻结参数补齐。

工程依据复用已核对的primary资料：[llama.cpp b10964 grammar README](https://github.com/ggml-org/llama.cpp/blob/b10964/grammars/README.md)说明response_format Schema约束输出但不注入prompt，模型没有Schema的可见语义。因此明确展示refuse的完整对象可直接消除“只在台词说拒绝、结构仍是craft/collect”的格式示范缺口。[Qwen3.5-4B模型卡Best Practices](https://huggingface.co/Qwen/Qwen3.5-4B)要求在提示中明确标准输出格式；本候选沿现有完整JSON示范追加一个分支，不改non-thinking、固定采样或输出预算。这些资料提供实现依据，不证明raw通过率会提升。

实际新增prompt159字符，静态计数不能替代模型tokenize。现实际最坏输入3146，3328上限余182token；根须通过现有实际请求模板/tokenize计数验证固定10的tokens/tier/dropped，再运行原冻结10。未删除现有唯一typed-limits说明或mandatory目录来预先凑预算。若有预算超限，不能提升3328/256、降低模型或删除目录/输入；当前草稿尚未获得token验收。

静态`semantic-neutral-refuse-static-proof.json`记录三例均完整8字段、新增字符数、32字台词及System之外前后源码完全一致。JSON例见`semantic-neutral-refuse-example.json`。完整目录24能力/158ID/507关系未改。

最窄实际验收仍区分raw、clear E2E、execute E2E、安全拒卡，以及input tokens/tier/dropped。子集不能代表60正式门/p95。若三个guard raw继续失败，照实记录；该提示不能代替现有UE守卫。
