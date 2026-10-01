# TASK-051｜操作、通行与验收基线交接

Owner／Reviewer：XLingyyy；无Issue。canonical 051＝原稿053。工作目录 `G:/GameFactory/Hearthward/.agent-local/task051`，分支 `codex/TASK-051-experience-baseline`，基线 `e1c44c49a88ce26125aa0e05fbe9d74b95ad7e58`（包含050及PR #55）。原Hearthward的027检出与未提交改动保留。

用户授权开始051设计任务。本轮只编制本地设计、操作表及验收协议，范围以任务JSON为准；新规则须Owner确认。尚无提交、推送、合并或发布授权。当前任务Active。

已读根工作流、启动／项目状态、两份指定文档、相关已批准设计和当前输入／设置／生存实现。开始时051尚无任务JSON，首次接手脚本报告缺文件；本次登记后重新执行。当前工作树只读取文档和源码，LFS资产保持指针，不作为可运行UE工程。

已完成[设计D1—D6](../design/DSGN-R23-input-traversal-acceptance.md)、[输入表](../planning/TASK-051/INPUT.md)、[54个逻辑固定对白cue](../planning/TASK-051/fixed-dialogue.csv)、[32项未来验收用例](../qa/TASK-051/CASES.md)及[联合性能／试玩协议和报告模板](../qa/TASK-051/PROTOCOL.md)。049的46句文本与运行表逐项一致，可复用20组录音；另8句051候选状态文本，全部录音尚未制作。新的通行／舒适性／门槛均标为候选，未覆盖旧批准。

L0 `validate_repo.py`通过，检查52任务快照、0错误；本地范围检查12路径、0越界；台词来源、唯一cue、B01—B32和模板JSON检查通过；`git diff --check`通过。详细命令与复核方法见[051报告](../qa/TASK-051/REPORT.md)。基线锁定的正式范围检查BLOCKED：main尚无051任务快照；本轮未伪造批准提交，也未改检查器。UE构建、运行、模型、联合性能和真人体验本轮均NOT_RUN。

README已替换旧“UI/fix未合入main”说明为PR #55已集成，纠正schema8及石斧hand_r现状，并添加051候选入口。CURRENT／R项只登记待审稿，R23／R24／R25没有关闭；全局PROJECT_STATE和START_HERE的既有漂移未扩入本单。

下一步为Owner明确确认／修订D1—D6，再将实际获准子范围登记为有效设计。后续施工需独立任务及相应源码／资产权限；当前051只有文档范围。状态保持Active，Owner与Reviewer按用户要求同为XLingyyy，未声明独立审查通过。没有Issue、没有新二进制锁；12个文档路径未提交，分支未推送。回滚本单可按这些明确文件逐项撤销；禁止清理原027工作区或删除工作树中的未交接文件。
