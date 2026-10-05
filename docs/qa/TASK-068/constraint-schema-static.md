# TASK068 constraint schema 静态检查

仅替换 HearthwardAgentContract.cpp 的 Schema() 生成段；Schema 外文本与检查时根源文件一致。

- 从实际 gameplay.json 读取77个注册ID，均为 ASCII [a-z0-9_]；从实际 capability 初始化读取24个 capability 的 Constraints。
- 按生产新增表达式构造静态 limits schema，以 Python 标准库 re 检查；未执行 C++ public Schema()、UE、llama.cpp 或模型。
- 静态断言总数：126。
- 正例：craft max:wood:0 / 3 / 99999 / 100000、repair no:wood / once:wood、collect source:S1、rule_proposal allow:wood；77个注册ID均接受 max:<ID>:3。
- 反例：100001、01、-1、3.0、未知ID、source:S2、craft ban/source、collect max、nature_collect source、超过4项；inventory/cancel等空Constraints capability只接受空limits。
- 原真实C08的 craft:arrows=3 和 collect:wood=3 被pattern拒绝；原真实C40的 cancel-001 被maxItems0拒绝。
- unresolved 保留原自由文本数组：maxItems4 / string maxLength120，pattern只位于limits分支。
- ValidLimit()/Validate()/Parse()/Describe()/Registry/v2/输入/测试期望未改动；规则恰1条、禁采当前目标等语义仍由既有Validate拒绝。
- 尚待根实际构建与C08/C40复测。格式约束不证明 max:wood:3 被保留、source=camp 语义正确、运输方向正确或p95达标。
