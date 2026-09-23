# TASK-028 单任务交接（2026-09-23）

状态：**Active，部分实现，未达到整单验收**。工作树 `E:\AiAgent\XLingGame\Hearthward-TASK-028`，分支 `codex/TASK-028-3d-assets-integration`，实施基线 `28e7c52e10e3988a19ca1e4b7e4fcf86ab2f3b93`。用户现已授权将当前进度提交、推送并创建PR；实现快照为 `f4d30df`，随后合入远端main `ba547c0`（PR #35、TASK-027），以便提供无冲突的审查分支。TASK-028仍未合并main；原始004源文件未修改。

远端进度：[草稿PR #38](https://github.com/XLingyyy/Hearthward/pull/38)，首次推送HEAD `c6549dd`，交接更新至 `dc26744`；80个LFS对象共118 MB已上传成功。PR建立时main又合入TASK-029/030，最新main为 `be9286f`。本分支已合入该版本并解决共享玩法/UI冲突，合并提交 `7ec921f` 的Editor构建成功，隔离路径49/49项通过；此交接随下一提交推送至PR。

## 已完成的接线

- [逐件清单](../assets/TASK-028/asset-manifest.json)盘点35件已有FBX候选与9个缺失装备模型条目。15件静态模型已导入 `Content/Hearthward/Assets/TASK-028/`，其中13件当前在场景/装备中使用；工作台木桌与篝火被TASK-030在main导入的同源Demo资源接管，本任务导入的2件重复网格降为已导入候选。每件含网格、材质与三张纹理；另有2个补缝专用纯色材质，共77个 `.uasset`，约111 MB。Color保留sRGB；Normal与Roughness按数据贴图导入，纹理最大尺寸2048。新资产受仓库LFS属性管理。
- 8件房屋模块由 `AHearthwardTask028CampHouse` 在自然营地附近运行时生成，不修改自然地图 `.umap` 或World Partition外部包。房屋有固定敞开的门、3级入口台阶、独立行走碰撞、屋顶/地板内衬与墙脚遮挡；内衬使用任务专属纯色材质，以免原模型贴图图集直接铺到立方体时出现方格。床、箱、椅、提灯为场景陈设，无新储物或交互规则。
- `workbench` 与 `campfire` 已在TASK-030的main版本使用同源Demo木桌/篝火模型，合并时保留main的尺度和新增床配方。本任务房屋、家具和斧头继续使用TASK-028专属资源；既有建造组件的同一配置驱动设施预览、完成与读档实例。
- 自然地图的既有玩法组件、建造页、制作页和维修页被最小接通；旧开发场景的原型伙伴、敌人与任务地标不被带入自然地图。营地发现/激活与旧自然档恢复一起处理。
- 石骨斧模型按既有 `weapon=axe` 且背包仍有斧头的状态显示，穿脱及存读档由原装备/库存事件重算。合入TASK-027后主角已改用Tripo骨架与动画；斧头仍挂在角色根节点的临时位置，尚未随骨骼手部和攻击/采集动作运动。

## 验证证据

- 引擎：UE 5.8.2，Windows，`HearthwardEditor Win64 Development`。最终C++变更使用 `E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ... -NoUBA`，退出码0，`Result: Succeeded`。一次UEClient构建在UBT写出Succeeded后关闭阶段挂起；改用相同目标的直接构建命令获得正常退出码。基线构建与最终构建分开记录。
- [导入报告](../qa/evidence/TASK-028/static-import-report.json)、[纹理修正](../qa/evidence/TASK-028/texture-fix-report.json)、[重开检查](../qa/evidence/TASK-028/reopen-inspection.json)及[内衬材质记录](../qa/evidence/TASK-028/backing-materials.json)记录15件源模型与2个任务专属材质的依赖、材质槽、尺寸和贴图设置。
- [完整路径测试](../qa/evidence/TASK-028/runtime-qa-full-route.json)使用新的隔离存档池，经正常标题页“新游戏”进入自然图，检查房屋模块与碰撞、原生移动输入进出门、建造篝火和工作台、制作页、斧头穿脱、手动保存/载入、返回标题页再“继续游戏”。该测试为了验证接线，仅向隔离测试档注入20木材和1把斧头；不代表正式自然地图有木材采集闭环。
- [DX12截图记录](../qa/evidence/TASK-028/visual-capture.json)及[外观](../qa/evidence/TASK-028/house-exterior.png)、[入口](../qa/evidence/TASK-028/house-entry.png)、[室内](../qa/evidence/TASK-028/house-interior.png)为PIE实时画面；测试只将观察相机移到固定位置，未修图。截图是工程自查，不代替Owner视觉验收。
- 测试入口：`python -X utf8 scripts/assets/TASK-028/inventory.py --check`、`python -X utf8 scripts/assets/TASK-028/run_runtime_qa.py`、`python -X utf8 scripts/assets/TASK-028/run_visual_capture.py`。后两者经GameFactory UEClient启动编辑器；再次运行前须保留并移走 `Saved/Task028/` 中的同名报告及截图。
- 合入远端main `ba547c0` 的TASK-027主角代码后，手工保留原报告，重新运行UE5.8.2 Editor构建（退出码0）和隔离存档的完整路径QA（48/48项通过）。新证据见[合并构建](../qa/evidence/TASK-028/task027-merge-build-summary.json)与[合并运行](../qa/evidence/TASK-028/runtime-qa-after-task027.json)。合并后的视觉截图未重拍；先前DX12截图只对应合并前的灰盒角色。
- 合入main `be9286f` 的TASK-029/030后，重新构建Editor成功（58动作、231.01秒）并重跑隔离路径49/49项通过，见[最新构建](../qa/evidence/TASK-028/task030-merge-build-summary.json)和[最新运行](../qa/evidence/TASK-028/runtime-qa-after-task030.json)。该路径注入20木材、4石材，石斧使用新游戏已有物品；四次失败尝试及原因保留在证据目录。最新基线视觉截图尚未重拍，无注入的自然采集→建造整条路径仍待测试。
- 仓库校验：`git diff --check`、任务脚本编译、相关JSON解析与逐件清单自检通过。`python -X utf8 scripts/validate_repo.py --task TASK-028 --base 28e7c52e10e3988a19ca1e4b7e4fcf86ab2f3b93` 仍失败：基线任务快照未列出两处必要的自然图UI入口文件 `HearthwardScreenContent.cpp` 和 `HearthwardScreenWidget.cpp`，校验器故报告 `OUT_OF_SCOPE`。用户已在本会话明确批准纳入这两处文件，工作树TASK-028 JSON也已记录，但校验器按设计只读取起始提交的快照。用户确认TASK-028的真实Issue URL与独立Reviewer尚未分配；TASK-026已有的Reviewer/Issue与测试ID错误也会被全仓校验报出。未伪造字段或绕过检查，集成前须建立能被基线门禁识别的批准快照并补齐任务归属。

## 未完成与阻塞

1. 用户确认没有其他可穿戴防具FBX。9个装备模型条目缺源，其中防具穿戴A5无法实施。TASK-027已通过PR #35合入main，提供角色骨架与基础动画，但尚无经过本任务验证的手持工具socket；斧头最终手部挂点、攻击/采集动作联动、跳跃穿插仍未实施，临时显示不构成A4完整通过。
2. TASK-030现已在main提供自然地图木材采集；本任务在旧基线的建造验证使用隔离测试材料注入，新main下尚需无注入联合回归，不把旧测试当作正式资源闭环证据。
3. 20件其余候选（鱼竿/袋/陶罐、两件敌人、15件动物）仍仅为源文件可用；对应玩法、动画或映射没有得到验证。房内陈设也没有新交互。
4. 尚未做同画质前后性能对比、Standalone长路线、旧档全量兼容、Owner视觉验收。房屋素材为 `TEMP_VISUAL`，源模型结构与UV有局限。A7和整单验收均未通过。
5. 本轮复核远端LFS锁，`Content/Hearthward/Assets/TASK-028/` 下锁数为0；未改动任何已有共享 `.uasset`、`.umap` 或外部包。新增77个任务专属资产已作为LFS指针进入提交 `f4d30df`，推送报告确认80个LFS对象、118 MB上传完成（含3张截图）。
6. 用户已批准两处UI文件纳入任务范围，但main任务JSON仍是旧版本，仓库范围门禁尚无法识别这次批准。以最新 `be9286f` 为base的定向检查报8项错误：TASK-027/028缺Issue和独立Reviewer、TASK-029报告3个相对链接指向缺失日志、TASK-028的 `HearthwardScreenContent.cpp` 被判OUT_OF_SCOPE。另一处UI文件已被main的TASK-029/030更新覆盖，不再属于本PR差异。Issue与独立Reviewer尚未分配；草稿PR不能直接合并。

## 继续执行

先补齐一件与TASK-027主角骨架兼容的可穿戴防具及缺失的核心装备源文件，并明确手持工具socket和动作接口，再做真实装备/攻击/采集/穿脱回归。在TASK-030已提供的自然木材采集路径上，重跑无测试注入的正常建造闭环。最后复核房屋视觉、固定路线性能、旧档与Standalone，并请Owner进行独立视觉验收。每项通过后再更新清单状态和任务验收，不以本轮局部PASS代替整单通过。
