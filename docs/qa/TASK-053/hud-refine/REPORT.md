# TASK-053 HUD测试版追加调整（背包改造前证据）

本记录绑定背包改造前的HUD源码；随后新增背包三栏界面，当前完整源码验证见[背包测试报告](../inventory-preview/REPORT.md)。以下218项及22项源码绑定只证明当轮实现，不作为后续源码PASS。

按用户2026-10-02追加要求，继续更新同一个[主界面UI测试.cmd](../../../../scripts/ui/主界面UI测试.cmd)。本轮仅为本机UE 5.8.2 Development测试；桌面安装版仍为0.2.0-preview.20261002.2，无Shipping打包或部署。Owner XLingyyy，分支codex/TASK-053-title-wheel，基线4db5789184fe38e041d62a1e68c8517338ea0b01，未提交／推送／合并。

生命、饱食、体力采用厚边槽、暗底空槽、顶部反光和弧面明暗，保留原红／黄／绿颜色、条槽前名称和实时数值。条高由首版10增加至22个设计单位；100%—150%文字设置下同步适配。

弟弟肖像、进度和当前状态移到条槽下方，任务移到弟弟下方。新收到任务显示标题与目标约五秒，随后整组消失；J任务列表保留完整详情。初始新游戏任务从加载完成后的HUD起算；周期刷新、返回HUD、追踪已存在任务或恢复存档不重放旧提示。UI只观察既有任务可用状态；同一批解锁多个任务合并为一次通知，完整列表仍在日志，没有改变接取、完成、奖励或保存规则。

发现指引匹配既有地点／物品名称及其真实世界坐标，使用玩家与目标的三维距离，在十米含边界以内显示；离开隐藏，重新靠近可显示。未知目标的残留发现提示隐藏，正常动作结果和倒地等玩法状态保留。探索记录与奖励不变。

首版删除教程小字／通知方框和四道具十字布局继续保留；选中居下高亮，其余左／上／右变暗且可识别。滚轮只改选择，原使用键和库存语义保留。此前标题、设置、存档及退出确认高亮修复保留，桌面程序与正式存档不变。

## 画面与启动

- [收到任务](../hud_26adea7c79fb/hud-task-received.png)：厚条槽在最上，弟弟居中，任务在其下。
- [五秒后](../hud_26adea7c79fb/hud-task-hidden.png)：任务标题与描述均消失，其余HUD固定。
- [药草膏](../hud_26adea7c79fb/hud-medicine.png)、[烤肉](../hud_26adea7c79fb/hud-roast.png)、[箭矢](../hud_26adea7c79fb/hud-arrow.png)、[投掷火罐](../hud_26adea7c79fb/hud-firepot.png)：四种选中布局。

以上六张均直接截取真实PIE游戏视口，保持正式序章出生点与镜头，没有图片编辑或HUD合成。逐张核对见[场景视觉记录](../hud_26adea7c79fb/visual-review.json)，150%字号核对见[大字号记录](visual-review.json)。双击同一测试cmd自动进入自然地图新游戏；每次独立GUID测试档池，依赖本机已有UE、Python和GameFactory环境。桌面“归火”仍启动原安装版。

## 实际验证

- [hud_refine_build_r2](../hud_refine_build_r2/build_result.json)：通过UEClient公开API构建HearthwardEditor / Win64 / Development，成功且无诊断。r1局部循环变量重名编译失败，已改名；截图尺寸初始化警告同时修正，失败原始记录保留于hud_refine_build_r1。
- [verify_248ca33b46eb](../verify_248ca33b46eb/report.json)：218/218界面检查、44张原生UI截图通过；其中[HUD](../verify_248ca33b46eb/hud-preview.json)51项、退出确认高亮26项。覆盖新任务通知与刷新不重启、原色厚条槽、弟弟／任务顺序、四道具与真实GameOnly输入、库存／生命更新、菜单／模态、旧任务恢复不重放、150%字号、720p及超宽。标题、八类设置与存档回归通过。
- [hud_26adea7c79fb](../hud_26adea7c79fb/report.json)：真实新游戏21/21检查、六张场景PNG。加载完成后初次取样任务剩余4.6892秒，等待5.2秒后为0且标题／目标元素完全不存在；任务列表详情存在，返回HUD不重播。[十米边界](../hud_26adea7c79fb/hint-range.json)11/11检查覆盖9.9米、10米、10.01米、垂直距离、远离及返回、未知目标与普通动作结果；临时位置／探索夹具在同一回调还原，并核对Gameplay快照及人物姿态相同。
- [hud_c91069bbba79](../hud_c91069bbba79/report.json)：实际Development -game -HearthwardHUDPreview自动进入自然地图，加载结束后三次连续窗口响应正常。三个自有测试进程均经UEClient正常停止；使用唯一GUID存档池并恢复开发环境用户设置。
- 上述三轮22项源码／UI配置／DLL指纹全部与当轮HUD实现一致，见[源码绑定](source-binding.json)；当轮授权范围核对见[本地审计](scope-audit.json)。仓库自检、33项工具测试、差异检查及正式基线检查的独立结果见[最终验证](validation.json)。
- 8项桌面程序／UI／正式玩家档及用户设置的SHA256保持相同，见[保护核对](preservation-result.json)；安装版本信息也保持相同，见[版本核对](installed-version-preserved.json)。

正式基线路径检查仍报告TASK-053在真实基线中没有批准快照，未作为PASS，也未通过未经授权的提交绕过。当前本地范围审计只核对用户授权范围，不替代正式基线快照检查。

物理Windows滚轮／键盘完整游戏体验和Owner视觉分析仍待用户继续测试；本轮是Development原生界面／输入管线及真实PIE场景验证，没有当前HUD的Shipping构建。首版历史见[HUD初版记录](../hud-preview/REPORT.md)。
