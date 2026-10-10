# TASK-104 应用与复验

这是源码、原创脚步音频、斧柄修形制作源和受控UE应用脚本的交付，不是已经重新打包的游戏程序。脚步合辑已获用户听感批准；最终UE编译、模型保存、实际动画/音效和Cook尚未执行。当前GitHub连接拒绝写入，尚无远端分支或PR。

## 1. 安全接入源码

以 `408b03ed175b096fedacfbc8845d3d5a82852a05` 为基线，在干净独立分支应用交付的Git bundle/patch；不要覆盖未保存的编辑器或其他工作树改动。交付包中的文件清单给出实际本地实现SHA与逐文件SHA-256。

- Git bundle保留完整任务提交及普通Git音频/制作源；不包含整个游戏已有LFS资源。
- `.patch` 可先 `git apply --check`，确认后再应用。若当前main已前进而检查失败，应审阅冲突，不能强制覆盖。
- `files/` 是本次变更的原始文件，供审阅/手动比较；不要将其误当作完整游戏工程。

本单新的15个WAV直接随源码保存，现有LFS资源仍按项目原流程取得。原角色、动作和斧头资产需要真实LFS文件，不能将指针当资源。

## 2. 原始斧头资源应用（必须）

仅C++握持修复不会自动改变旧`.uasset`的粗柄几何。`art_source/TASK-104/grip/`提供修形OBJ及精确顶点差量，`scripts/equipment/task104_apply_axe_grip.py`在UE中将差量应用到原斧头包，保留其现有资源路径、UV、拓扑、材质与Grip/BladeBase/BladeTip插槽。当前交付中该UE二进制应用未执行。

使用具备本项目工具链的UE编辑器与项目既定UEClient；先关闭PIE，确保无未保存的Content/地图，在Git LFS服务核对并取得该斧头包的本人写锁：

`Content/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.uasset`

在新的编辑器进程中运行脚本，并显式传入 `-Task104AxeApply -Task104AxeLockId=<真实锁ID>`。不要编造锁号；脚本会验证服务返回的归属、原资源SHA和原顶点，再只保存这个包。没有Apply标志时是只读复核，原资产尚未应用时应报告失败，不能将它算成功。

应用后检查 `Saved/Task104/axe-grip-apply.json` 的 `ok`、保存包列表与几何/UV/材质/碰撞/插槽签名。随后在新编辑器进程不带Apply再运行，检查probe报告；脚本拒绝把后续未知改动重新写成新的可信基线。报告缺失、失败或未知时不要继续发布。

## 3. 可在无UE环境执行的检查

从仓库根目录：

```sh
python -m unittest discover -s scripts/tests -v
python scripts/audio/generate_surface_footsteps.py --check
python scripts/validate_repo.py
```

地形重建校验 `python scripts/audio/prepare_footstep_terrain.py --check` 另外需要既有制作流程的Pillow/numpy和三个真实LFS地形遮罩。已有runtime表本身随交付提供，不需要玩家执行生成器。

仓库自检目前有4处基线历史QA链接缺失；本单保留原失败证据，没有删除检查或伪造旧文件。`--task TASK-104 --base <原main>` 还因基线尚无该新任务快照而拒绝正式范围认证；这不是UE游戏验证。

## 4. UE复验与打包

先应用并重开核对斧头资源，再使用[UE检查入口](run_ue_checks.py)运行本项目Editor构建、`Hearthward.Iteration.Task104.`及既有`Hearthward.Iteration.Task099.`自动化。具体Windows工具链与正常游戏检查列表见[验证记录](REPORT.md)。必须实际发现并执行测试；源码中存在测试不等于通过。

使用独立测试档池，观察兄弟持斧待机/行走/奔跑/采集/轻重击、装备切换、远程/救援/倒地时的手指恢复；核查实际BladeBase/BladeTip扫掠、命中与空挥。只有本单持斧姿态可改变，其他武器动作和战斗时序不应改变。

脚步实际路线：石堡木地板→石台阶→草地；营地木地板/三阶入口也应是木声。自然地形参考点（厘米）：草 `(-98000,-75000)`、S1土 `(-110000,-70000)`、河边土 `(60000,0)`、北部岩面 `(-30000,140000)`。按真实地面接触触发，保持未知回退、去重与音量生命周期。

最后执行项目正常Shipping构建/Cook/运行流程；当前Linux云环境无法承担完整UE工具链/渲染，也未移植Windows专用本地AI可执行文件。不要把本源码包当作新发布安装包。
