# TASK-049｜角色资产接入

使用仓库现有TASK004 Tripo短刀兵、重甲兵带骨骼FBX及其材质，不调用付费生成、不改源文件。来源任务、参考图和素材ID沿[敌人源索引](../../../art_source/TASK-004/Tripo/敌人/敌人模型与骨骼索引.md)；本次引擎导入不构成额外授权声明。

Campaign目录共34个uasset：两组原始及Runtime骨骼网格、Skeleton、纹理／材质，Idle／Walk／Run／Attack重定向动画，以及源动作／IK Rig／Retargeter。执行脚本为[import_enemies.py](import_enemies.py)、[retarget_enemies.py](retarget_enemies.py)，导入结果见[import-assets.json](import-assets.json)，8段导出结果见[retarget-assets.json](retarget-assets.json)，LFS锁核对见[lfs-locks.json](lfs-locks.json)。导入及导出由UEClient启动的编辑器执行。

Runtime网格统一尺寸与原点；动画复用已有弟弟动作并通过IK Retargeter导出。短刀兵用于普通兵及临时射手外形，重甲兵用于重兵；实际血量／攻击／投射物逻辑按兵种区分。族人和受保护角色暂复用弟弟模型，房屋复用TASK028，任务匣复用已有木箱，旗帜为基础网格。

上述为TEMP_VISUAL。没有新增录音、射手专用弓持握表现或最终过场镜头，不能按角色最终美术验收。PIE近景见QA报告链接；截图仅证明本次场景和姿态，不代表所有动画帧、LOD或性能均通过。

完成态画面：[故乡恢复](screenshots/reclaimed.png)、[任务日志已完成状态](screenshots/all-quests.png)。任务日志截图显示当前选中任务；全23项完成由尾声PIE断言及存档记录证明。
