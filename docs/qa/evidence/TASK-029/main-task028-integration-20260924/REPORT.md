# TASK-029 + 最新 main TASK-028/030 组合复验

日期：2026-09-24。Windows，UE 5.8.2 Editor Development，隔离 `-HearthwardSaveTestPool`。把 `main@97af300bd801917ece85c21b87d5ce482bf50052` 合入 `codex/integrated-latest-20260923@8c968519cbcf0b699f1120ae0b146481a5a5061f`；测试在提交前的同一合并源码树上运行，之后只改文档。

## 合并内容

- 引入 main 的 TASK-028 营地房屋、家具和石骨斧资产；C++ 自动融合，README 冲突人工合并。AI、UI/输入、跟随、近战及 TASK-030 自然采集逻辑保留。
- 石骨斧为装备状态驱动的临时 Capsule 挂接，手部 Socket 与动作适配仍未完成。
- 新增 `.uasset` 均继承 main，没有手工二进制合并。当前工作树的 77 件 TASK-028 `.uasset` 均为实际文件，非 LFS pointer 文本。

## 本轮验证

| 项目 | 结果 | 证据 |
|---|---:|---|
| UE Editor Development 构建 | PASS，9 actions | `Build.bat HearthwardEditor Win64 Development` |
| Python 单测 | 31/31 PASS | `python -X utf8 -m unittest discover -s scripts/tests -v` |
| Native `Hearthward.*` | 43/43 PASS | `Saved/Task029Integration/task028-merge-native.log`，SHA256 `66D6A1CDE9D99531F4E3B2CC7252A36CE0F74A8B9A2F600E6710E560740E1753` |
| TASK-030 Demo | 70/70 PASS | [demo.json](demo.json)，日志 SHA256 `84055D39BA1AAC20E7E0387C34B2FD9407704F9F6577C4A433A84616FA9F57F2` |
| 无授物自然玩家路线 | 51/51 PASS | [natural-route.json](natural-route.json)，日志 SHA256 `87AD401D00D3AB30364DDDD69E11B81A45386E8FF7200EDA4F1A780D35E80BCC` |
| TASK-028 房屋/石骨斧路线 | 49/49 PASS | [task028-asset-route.json](task028-asset-route.json)，日志 SHA256 `5B418A517D4282007C41E6728F5F0CDFECB038973540D85E5E0EC1A7186EC308` |
| Repository validator | 4 errors | canonical TASK-027、TASK-028 各缺 reviewer 与真实 Issue URL；来自 main 元数据 |

TASK-028 路线使用隔离档和测试材料注入，不代表正常玩家材料循环；51 项无授物路线单独证明正常入口。Native 日志统计 43 次 Success、0 次 Failure。

上轮 `main@be9286f` 组合的 UI 62/62、疾跑跟随 8/8、攻击 10/10、AI runtime smoke 23/23 见[前轮报告](../main-integration-20260924/REPORT.md)，不冒充本轮重跑。当前组合的真人键鼠、自然敌人命中、石骨斧手部动作、跳跃手臂、当前 Qwen 耗时、Shipping 包及独立评审均 NOT_RUN。合并后还须按 main 新 SHA 记录集成验收。
