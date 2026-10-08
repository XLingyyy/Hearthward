# 弟弟装备呈现增量

2026-10-08，UE 5.8.2 / Blender 5.2.0 LTS，任务分支 `codex/TASK-084-103-iteration`。准确单包范围基线 `f6aaec42ff92b9b2db7048a9e4ce8dd13acf1d1c`，受测实现提交 ca8982eb5c2df81883390c798979be0ee1a951bc。TASK-095保持Active。

覆盖弟弟13种近战装备，使用真实实例GUID、耐久和生存状态；工作、倒地和施救时隐藏。新增原创右臂携带姿势，复用现有角色骨架和武器资产。原石斧保留原挂点。实现与来源见 `docs/assets/TASK-095/BROTHER_WEAPONS.md`。

最终Editor构建成功，`Hearthward.Iteration.Task095.WeaponInstancePresentation` 1/1通过、零测试警告，覆盖本轮弟弟13种近战装备、卸下/破损，以及原测试内玩家17种装备、弓弩和长枪检查。实际命令见本目录verify脚本，原始结果见build.json、native-index.json。

实机六件装备待机采样及长刃正/侧视共八张图见frames，真实绑定/世界缩放见preview.json。首次挂接在原待机手位上，长刃和战斧穿过肩部；失败图见failed-carry。修正为身体右前侧的掌心位置后，长刃、战斧和长枪与肩部保持分离。侧视图有旁边守卫遮挡下半手位，斜视和正视用于补充核对。没有把这组静态采样当作连续移动或攻击接触验收。

新动画导入原 `SK_Brother_Skeleton`，一秒固定携带姿势，源掌心误差约6.83e-8米；它按右臂分层，不是全身步行动作。源结构/导入结果见author.json、import.json。未修改伤害或1.2秒攻击间隔；0.25秒可见前摇仍等待Owner设计答复。弟弟专用长枪攻击、动态接触、连续移动、正式路线、Cook及Owner视觉仍未闭合。

本轮夹具为PROTOTYPE_ONLY隔离PIE，采用生产装备接口，不计真实OS键鼠或正式路线。旧preview.json的fixture字符串含“attacks”，但实际attacks数组为空；本报告明确仅六件待机装备和两张补充视角。未声称执行攻击验证。

最终范围检查36条路径、零错误；git diff --check通过。工具层未修改，沿用同会话33项通过结果。
