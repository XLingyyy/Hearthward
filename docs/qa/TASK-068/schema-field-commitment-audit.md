# TASK-068 Schema首字段承诺：只读调查与候选

2026-10-04。根基线为 `.agent-local/task051` 当前 `HearthwardAgentContract.cpp`／`HearthwardLocalAISubsystem.cpp`。本代理只读根实现及真实报告，下载官方b10964主源到自身QA目录，准备Schema-only补丁草稿；未改生产、运行UE／模型／build或执行Git。

## 实际证据

根最新 `docs/qa/TASK-068/vulkan-semantic-prompt-red-10-results.json` 的冻结诊断子集为5/10 raw通过，仍未达到60表达式门槛。`semantic-prompt-red-10-field-evidence.json` 仅抽取该报告原始字段和raw结果，未修正JSON、输入或期望。

|样本|真实raw的执行字段|真实raw的unresolved|结论|
|---|---|---|---|
|A01「帮我采些木材」|collect／wood／quantity1|缺确切数量，并新增采集地点疑问|输出可执行意图后又明确声明缺量；raw失败|
|U01「新采负三份木材带回仓库」|collect／wood／quantity3|负数数量无效，请确认|把负数改成正数，后面仍说明无效；raw失败|
|U03「新采三十三份木材带回仓库」|collect／wood／quantity32|33超32，声明实际执行32并询问剩余1|数量被截至上限，后面仍指出超限；raw失败|

三项e2e安全拒绝通过，不能计为raw理解通过。三个raw对象以及其余七项的字段顺序实际一致：intent → item → quantity → mode → source → limits → unresolved → npc_line。

当前Schema():179–205允许写操作的unresolved有0–4项；rule_proposal limits有0–4项、unresolved有0–4项。Validate():231–232要求全部C.Writes的unresolved为空，rule_proposal的limits恰1项且unresolved为空。此差异由当前源码直接确证，无需假设模型思路。

Parse():237–244按字段名逐个TryGet，检查恰8字段以及原有数值／数组范围；没有key-order依赖。Schema重排和收紧到既有Validate范围不改变Parse、权限、确认、epoch／serial或锁参数。现有 `Tests/NPCAgentTests.cpp:32` 已覆盖unresolved阻断执行；:66只核对Schema结构，尚未覆盖两项数组差异或实际序列化顺序，不应重复注册同一业务拒绝用例。

## b10964公开源码链

官方tag文件已保存于本目录 `primary-b10964/`。这是此次从官方原始文件下载的参考源码，未冒称本地binary的完整build tree；没有计算hash或重新构建。以下行号按保存的原始文件计，浏览工具排版行号可能不同。

1. [common/json.h](https://github.com/ggml-org/llama.cpp/blob/b10964/common/json.h):18–25明确common_json保持对象添加顺序；[common/json.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/common/json.cpp):14、201–205以nlohmann::ordered_json解析。server-common.h:24也将server json别名绑定到common_json。
2. [common/json-schema.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/common/json-schema.cpp):133–151从properties.items()按对象顺序追加IR vector。required是unordered_set，仅用于判断成员是否必填。234–236把oneOf／anyOf都转为alternatives union；本项目每分支intent不同，实际分支互斥。
3. [common/json-schema-to-grammar.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/common/json-schema-to-grammar.cpp):698–747按properties vector先积累required_props再按序拼出object规则。当前八字段均required，故properties顺序就是输出顺序。required数组仅重排不会改变顺序；内部_rules的map按规则名排列定义文本也不会重新排序object输出字段。
4. 同converter的array路径:923–930采用真实min_items／max_items；build_repetition():18–20在max0时返回空内容，实际只允许空数组；min=max1时仅允许一个item。`json-schema.cpp:192–193`确实读取minItems／maxItems。
5. [tools/server/server-common.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-common.cpp):1185–1195提取本项目response_format.type=json_object及schema；[server-schema.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-schema.cpp):259–267调用json_schema_to_grammar，按OUTPUT_FORMAT grammar约束生成。未提出改请求字段或换生成接口。

**后端边界：** converter:992–1001在编译启用LLAMA_USE_LLGUIDANCE且非force_gbnf时返回 `%llguidance / %json`，不会经过上述GBNF对象排序生成器。[b10964 CMakeLists](https://github.com/ggml-org/llama.cpp/blob/b10964/CMakeLists.txt):146默认LLAMA_LLGUIDANCE为OFF；默认值不能单独证明根现有binary的编译开关。本次未启动server或取build日志确认该开关。当前真实raw全部跟随现有properties顺序，与GBNF链吻合；下一次排序候选必须核对actual raw确实以unresolved开始，不能仅根据C++对象构造宣称运行时已重排。若顺序未变，先检查实际发送schema／grammar后端，不更改期望或私自强制另一个后端。

## 分支何时被排除

[src/llama-grammar.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/src/llama-grammar.cpp):1020–1054、1472–1515对已接受token的字符逐个匹配，每步只保留surviving grammar stacks。union不会在root提前任选一个分支；共享前缀可同时保留多个分支。字符到达互异的intent常量前缀后，其余意图分支会逐步消失；collect已输出后不能再生成clarify常量，也没有修改先前已输出token的机制。

若Schema与Validate先对齐，且unresolved为第一字段：

- 非空unresolved数组第一个字符串开始的引号，即与写操作／rule_proposal只能关闭空数组的规则冲突，这些分支随即被排除；后续只能选择仍兼容的非写意图。
- 第一字段输出空 `[]` 时，合法写操作和非写分支仍可存活；模型仍可能漏掉歧义、发明数量或选择错误意图。这一改动不保证理解正确。
- Schema仅能限制JSON结构和已表达字段的关系，不能推断原话确切数量、最终运输目的地、材料授权或物名。C09新增max、C18错误最终终点也不能由该候选自动解决。

**推断：** 当前强制先intent、再有下限／上限的quantity，可能迫使模型在表达疑问前完成执行分支，随后以晚出的unresolved补充拒绝理由。三项raw和grammar机制支持此解释；没有模型内部过程证据，尚不能将它认定为三项语义失败的唯一原因。

## 最小候选与单变量验证

准备稿均相对根本次当前源码生成，真实源行尾LF；仅修改Schema()，对外部函数文本已静态比较保持一致，没有应用补丁。

**A：`schema-unresolved-first-candidate.patch`。** 保持现有全部值域／数组上限／模式／限制pattern／required成员，只将properties排列为 unresolved → intent → item → quantity → mode → source → limits → npc_line。使用一个局部FJsonObject按明确序列接收现有八个共享字段；无公共helper、新字段、额外prompt或parser修改。required列表保留，避免将列表排序误当grammar排序。

先只应用A，在原10项冻结诊断上确认actual raw字段顺序和业务结果，以验证首字段影响。A仍允许非法writes+unresolved组合；若继续出现，只保留真实失败，不降低raw判定。

**B：`schema-validator-alignment-candidate.patch`。** 两处当前规则对齐：

- unresolved：C.Writes或rule_proposal分支maxItems=0，其他分支保持0–4项及120字符上限。
- rule_proposal limits：minItems=maxItems=1，保留既有限制类型／物品／整数pattern；其他分支不变。

B草稿独立相对当前根基线，与A修改位置不冲突；在A稳定后仅叠加B，再比较同一冻结10项。若根直接合并A+B，应记录为耦合Schema候选，不能将结果单独归因于排序。

**避免先单测B并把结果当修复：** 保持intent先输出时，B会强迫执行分支的晚出unresolved为空；模型原先明确识别的疑问可能被压掉，错误数量仍成为结构合法提案。验证必须继续看原始语义以及确认前副作用，不能仅以Validate不再报错计成功。两步候选均保留既有Validate／Parse和确认权威，不做raw后纠正。

根最窄验证范围：当前实际Schema公开生成／序列化读取，检查写分支unresolved max0、rule limits恰1及rule unresolved max0；随后原冻结10项真实模型A/B，保留每项raw、actual key order、reason、raw／e2e／执行及未确认物品变化。模型4B／3328／256、单并发／线程／GPU层、输入／expected和判定门槛全部保持。诊断10项不足以取代正式60项验收或暖性能门槛。

## 本次产物与限制

- `semantic-prompt-red-10-field-evidence.json`：真实10项的精确raw和字段顺序摘录，5/10结果保留。
- `schema-unresolved-first-candidate.patch`、`schema-validator-alignment-candidate.patch`：仅QA草稿，没有生产应用或C++编译结果。
- `primary-b10964/`：官方源码参考缓存；不新增运行依赖或框架。
- 没有UE／模型／build／Git、输入改写、预算变更、权限扩张或正式PASS结论。
