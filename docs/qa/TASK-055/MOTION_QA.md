# TASK-055动作候选检查

2026-10-03。源为已有本地knight61 FBX，Hero、Brother各12个候选AnimSequence导入成功。现有两骨架与新增源/序列锁属于XLingyyy，未由子Agent写资产。

真实Hero与Brother SkeletalMesh分别单节点播放slash、hit_to_side、fall、swim，各12项加载/骨架/关节运动检查通过。Saved/Task055/motion-hero保存8张两视图截图；主Agent已查看4张，最终灰盒画面曝光可读、角色肢体连续。该场景关闭角色移动和Presentation摄像机更新，仅用于资产姿态检查。

候选slash长6.583s，hit_to_side1.25s，fall3s，swim5.708s。完整动作分段、有效打击窗口、Fall的倒地用途和Swim水中姿态仍需核对；尚未接入正式AnimInstance或认定战斗/游泳动作验收。初次自然地图摄像机受Presentation覆盖，第二次灰盒临时灯过曝，已纠正验证夹具后重拍，最终结果不依赖上述无效画面。组件bounds使用世界坐标，未误称局部bounds。

复跑：`python docs/qa/TASK-055/run_motion_pie.py --script verify_motion_hero.py --label motion-hero`。工程QA仅用UEClient公开生命周期，无地图资产保存。来源许可仍由074核实，当前档案未确认许可。

2026-10-04恢复核对：Brother同样12项通过，已查看slash正面与swim侧面两张；正面部分被场景既有Widget遮挡，侧面可读。两角色各8张截图，未把抽查视图作为整段动作验收。25个新源／序列锁和两现有骨架锁保留。
