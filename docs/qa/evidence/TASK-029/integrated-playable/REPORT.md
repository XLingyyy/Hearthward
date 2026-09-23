# TASK-029 / TASK-041 / TASK-042 双工作树集成复验

日期：2026-09-23。分支：`codex/integrated-latest-20260923`。移植起点：`66d0107a3eb04646f29cd887322c543076feaee6`，其来源为 `main@ba547c0` + PR #34 `25d53d8` + TASK-029 latest-main AI 核心。第二来源为 `codex/TASK-042-playable-loop@a17c1e2` 及其未提交工作树；仅选择性移植，未整体合并旧 AI 底座。远端 main 后续前进至 `be9286f`（TASK-030），本轮未并入。

本轮验证的源码提交为 `4dae443094346f7d1ed8e4a62ba11370dc2165d7`；测试在该提交前的同一源码树运行，提交只归档了已验证代码和原始结果。

## 本轮改动

- 页面保留跨页返回栈与分类位置；Tab/T/R/F6 快捷键与关闭路径一致。确认框独占 Enter/F/鼠标操作，刷新后键盘焦点落在可用控件。自然营地的地图与任务日志可访问。
- 交互目标等距时稳定排序，HUD 提示与实际执行目标对应；自然地图优先绑定已布置的树实例作为有限采集点，失败时保留原型标记回退。
- 左键攻击只经 Enhanced Input；空挥播放动作，不造成战斗状态或耐久损耗；真实命中才伤害并扣耐久。伙伴指令 30 米外有中文反馈，跟随速度随玩家实际奔跑速度提高。
- 保留 TASK-029 的自然旧档升级、Schema 3、模型和上下文权威边界。没有引入 TASK-042 的旧存档 mismatch 拒绝、自动生成的 RHI/音频配置或多余的新游戏入口。

## 当前源码验证

| 验证 | 结果 | 边界与原始证据 |
|---|---|---|
| UE 5.8.2 Editor Development | PASS | `Build.bat HearthwardEditor Win64 Development`；正式源码在移除临时诊断日志后再次构建成功 |
| Python unittest | 31/31 PASS | `python -m unittest discover -s scripts/tests -v` |
| 原生自动化 `Hearthward.*` | 42/42 PASS | `Saved/Task029Integration/native.log`，SHA256 `D7B3D3BA87453C27BF35786A64D74F7FC0D249933E9BCC907621DBD497B21415`；最终源码与此轮测试时相同 |
| UI 返回、焦点、模态 | 62/62 PASS | [ui-result.json](ui-result.json)，隔离 PIE / HUD API，非物理按键 |
| 正常新游戏自然地图路线 | 50/50 PASS | [natural-player-route.json](natural-player-route.json)：行走采集、建工作台、制作、仓储、地图/日志、保存；非真人实键 |
| 8 秒疾跑跟随 | 8/8 PASS | [long-follow-result.json](long-follow-result.json)：玩家移动 3589 cm，伙伴距离 203 cm；隔离 PIE 输入模拟 |
| 装备斧头攻击 | 10/10 PASS | [attack-result.json](attack-result.json)：空挥动画/无磨损，显式开发敌人夹具中真实命中/扣耐久；自然地图没有敌人，未做物理鼠标验收 |
| TASK-029 runtime smoke | 23/23 PASS | [runtime-smoke-result.json](runtime-smoke-result.json)，显式开发夹具和隔离存档池 |
| 全仓 repository validator | 2 errors | canonical TASK-027 缺 reviewer 和 Issue URL；与 checkpoint 基线相同，未越权修改 |

测试脚本与结果一同保存于本目录。攻击验证的前两轮因夹具直线路径被灰盒阻挡及脚本未等待 0.65 秒攻击冷却而失败；定位后使用显式开发夹具近距离站位，并等待实际冷却。失败记录保留在本地 `Saved/Task029Integration/`，不计入最终 PASS。

## 仍需验收

- 真实键鼠连续游玩，包括左键对实体敌人造成可见伤害、X 跟随、T 对话响应时间及页面焦点；本轮自动化只证明对应 UE 调用路径。
- 自然地图尚无可攻击敌人；可见斧头网格/Socket 未接线。跳跃时手臂异常属于受 LFS 锁保护的动画资源，本分支未修改；仍需资产 Owner 接线与视觉复测。
- 真人 Reviewer / Owner、Issue URL 与 canonical TASK-027 流程元数据不由本分支代填。PR #34 仍是旧 head，不能替代本轮分支的审查。
- `be9286f` 的 TASK-030 资产及玩法是独立后续集成；本轮构建与测试不覆盖它。
