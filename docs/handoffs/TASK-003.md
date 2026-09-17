# TASK-003 交接

## 版本与状态

- 日期：2026-09-17；Owner：XLingyyy；唯一写者：当前 Codex 会话。
- 分支：`codex/TASK-003-third-person-bootstrap`。
- 基线 HEAD：`ff5ff7138991e316a4af1dd53f4b583d615d9acd`。
- tested_commit：**UNCOMMITTED_WORKTREE**。该 HEAD 仅为基线，不包含本次游戏实现；不能把证据标成该 SHA 的 PASS。
- 用户授权本地执行 TASK-003。独立评审人未指定；GitHub Issue 创建返回 403 `Resource not accessible by integration`。
- 工作流维持 Blocked；无 PR、自审通过或合并。2026-09-17 用户已授权本次实现连同资产任务单提交推送；提交记录在后续交接更新中绑定。

## 实现与设计对应

根目录 UE 5.8.1 C++ 工程，Game/Editor Target 均使用 V7；启动地图为 `/Game/Hearthward/Bootstrap/L_Bootstrap`。
Character、CharacterMovement、SpringArm、Camera 和 Enhanced Input 均复用引擎组件。
Q002 对应单人第三人称；Q177 对应无负重常态速度 350 cm/s。未修改原始 DOCX、设计归档、OPEN 问题或契约。

WASD、鼠标仅为测试输入。镜头臂长 400 cm、目标偏移 Z=60 cm、胶囊半径 34 cm／半高 90 cm、
转向速度 500°/s、步行制动加速度 2000 cm/s²均为本任务工程初值；鼠标沿用引擎默认输入比例。
角色由引擎基础几何体组成。地图为 40×40 m 平台、边界墙、中央碰撞墙及距离条。
无冲刺、跳跃输入、耐力、战斗、伙伴、库存、存档或正式关卡。R09、R23、R24 保持未定。

## 验证证据

- [build.json](../qa/evidence/TASK-003/build.json)：真实 Development Editor / Win64 构建退出 0，22 个构建动作成功。
- [pie-results.json](../qa/evidence/TASK-003/pie-results.json)：同一编辑器内连续两轮 PIE，每轮 9 项，共 18 项通过。
- 四向及斜向稳态均为 350 cm/s；松键稳态为 0，再输入恢复 350；撞墙后速度为 0，中心 X 约 915.9 cm，符合墙面 950 cm 减胶囊半径 34 cm。
- 镜头动作注入产生偏航及俯仰变化。实际鼠标操作另见 [操作前](../qa/evidence/TASK-003/mouse-before.jpg)、[操作后](../qa/evidence/TASK-003/mouse-after.jpg)，在额外一轮 PIE 中完成，Esc 正常结束。
- [verify_pie.py](../qa/evidence/TASK-003/verify_pie.py) 可复跑上述两轮测量；测试选择当前角色已映射动作对应的输入子系统，避免前一轮对象尚未回收时误选。
- [PIE 日志摘录](../qa/evidence/TASK-003/pie-log-excerpt.txt) 保留启动、生成、结束及 Python 信息。
- [键盘录屏](../qa/evidence/TASK-003/keyboard.mp4)、[输入事件](../qa/evidence/TASK-003/keyboard-actions.jsonl)、[录制报告](../qa/evidence/TASK-003/keyboard-report.json)：UEClient 通过 Windows SendInput 执行 WASD、斜向、停步及撞墙，18 次按下／释放事件均成功。已检查起始、移动中和最终撞墙画面。
- 录制另用 Editor `-game` 模式，共取得 65 帧；以 12 fps 编码后视频约 5.42 秒，属于抽帧回放，不能据播放时长推导实际速度。定量速度以两轮 PIE 报告为准。
- 仓库结构和任务范围检查通过；仓库工具自测 29/29 通过，见 [repository-checks.json](../qa/evidence/TASK-003/repository-checks.json)。这些检查与 UE 运行测量分别报告。

定量脚本的早期尝试因 Python 包装层 API、受保护属性及运行时对象选择失败，已修正；这些失败未被计入通过用例。
引擎初始化阶段日志仍出现 `LogAutomationTest: Error: Condition failed`，发生在 PIE 之前，未阻止加载和操作。
本任务未确定这些初始化日志的根因，不能把当前记录描述成“全日志无错误”。Python 弃用提示属于验证脚本使用的 EditorLevelLibrary API。

构建与复现入口见 [BUILD_AND_TEST](../qa/BUILD_AND_TEST.md)。本次没有运行打包、两机复现或完整游戏闭环，不代表 M0 完成。

## 资产与后续流程

四个资源位于 `Content/Hearthward/Bootstrap/`，受现有 LFS 规则管理；锁记录见 [lfs-locks.json](../qa/evidence/TASK-003/lfs-locks.json)：

| 资源 | 锁 ID |
|---|---|
| L_Bootstrap.umap | 51800256 |
| M_Floor.uasset | 51800277 |
| M_Marker.uasset | 51800299 |
| M_Wall.uasset | 51800321 |

锁保留至正式集成交接。已获提交推送授权，将 Source、Config、Content、插件源代码及证据作为同一任务提交，
记录该提交 SHA 并确认 LFS 对象上传；随后由独立评审人评审。当前 A5 的同一 SHA／远端复现尚未完成。
本机 Windows 的既有 `config/` 与引擎 `Config/` 共享目录；新增 ini 的 Git 路径保留 `Config/`，工具链仍保留 `config/toolchain.lock.json`。
