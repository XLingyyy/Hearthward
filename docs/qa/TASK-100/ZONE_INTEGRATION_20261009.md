# TASK-100 四区场景接入｜2026-10-09

状态：四类模型、16 处运行时布景及局部通行完成；整单保持 Active。完整连续主线、逐条支线、胜利后正常保存继续尚未通过本轮正常输入验收。

## 实现

- Blender 5.2 LTS 通过当前会话原生 MCP 制作四类模型，源文件 `art_source/TASK-100/ZoneArchitecture.blend`，制作脚本和尺寸、落点清单同目录。现有 `WorkshopEntry.blend` 保留。
- 河门石门、作坊棚屋、住区双门木屋、议场开放柱廊各布置四处。作坊复用 098 工作台／熔炉／仓库模型作陈设；石材和木材使用 096 的世界坐标材质。陈设不新增生产、资源或交互入口。
- 四模型分别含 39、43、55、24 个凸碰撞块。门洞两侧、屋顶和柱子独立碰撞，中央通道保留；资产尺寸经 UE 实测为厘米单位。
- 原房屋坐标存在陡坡，首轮实机发现一栋住区门楣阻挡、多个地脚悬空。以正式地图射线测量筛选平地，将七处展示落点平移 15–68 米，全部仍在原控制区内，并避开原巡逻线、出生点和交互位置；地脚按模型边界最低支撑点落地。原配置、入口、旗点、主支线条件、兵力、奖励和存档字段没有修改。
- `DefaultGame.ini` 为动态加载的新模型登记 AlwaysCook 目录。既有预览包 11 尚不包含本轮新增资产；本轮没有另行制作 Shipping 候选包。
- 旧 067 夹具在已占领旗点要求显示交互，与用户已确认的“完成后隐藏”冲突。42.5% 与 1/4 的原断言移至未占领的工坊旗点，另断言占领后交互提示为空。

## 授权与版本

用户明确要求先执行 TASK-100，资产使用已连接 Blender MCP；承接此前提交／推送授权。范围先登记于 `73becfa8`，Cook 范围补充于 `1be34848`，相关旧夹具范围补充于 `a5df1cd4`。最终范围检查以包含全部批准路径的 `a5df1cd4` 为基线。准确包清单见 `docs/assets/TASK-100/PACKAGE_SCOPE.json`，七个 UE 包和新 Blend 源的 LFS 锁均已取得，保留至整合。

受测实现为上述基线之后的本轮工作树；最终成果提交号登记在本单 handoff。没有计算额外文件哈希。

## 本轮验证

|检查|实际结果|证据与边界|
|---|---|---|
|Blender 前／后／左／右／顶视|四类模型五视图已检查|`zone-integration/blender-orthographic.png`；另保留制作源|
|公开 UEClient 导入|4/4 成功，资产有碰撞|`zone-integration/import/*.json`|
|Editor Development|成功|`zone-integration/build.json`|
|Task100 + Campaign049 + Campaign067|12/12 Success，0 errors|`zone-integration/native-green.json`；7 个既有测试世界销毁 warning 保留，不表述为零警告|
|门洞与侧墙物理碰撞|4/4|真实 StaticMesh 玩家尺寸胶囊 sweep；中央贯通、侧面阻挡|
|首轮自然地图穿行|15/16，住区第 0 栋失败|`zone-integration/world-red.json`；原坡度导致门楣净空不足|
|修正后自然地图穿行|16/16|`zone-integration/world-green.json`；每栋实际 AddMovementInput 穿行超过 8.5 米，地面五点高差最大 21.88 厘米|
|区域截图|独立固定相机检查|只作为布景与地形证据；按实际相机位置校验，使用正式夜间光照|

实机检查使用隔离存档池。建筑之间采用诊断定位，穿门使用角色真实移动；测试未清敌、未赠送资源、未改任务阶段或将这条检查路径计为正常通关。早期 QA 脚本的相机生成／HitResult Python API 错误已经修正；最终通过记录对应修正后的脚本。`world-04` 的重复相机截图不作为四区视觉证据，仅采用独立固定相机的最终截图。

本次默认 Editor 启动停在 Zen 服务就绪等待，原 UEClient 在超时后关闭自己启动的实例。后续只给本轮 QA 进程传入 `-DDC=InstalledNoZenLocalFallback`；未修改全局缓存、工程默认配置或他人进程。Blender 可执行 Python、建模、导出和渲染；插件 `get_addon_status` 的缺失模块错误没有影响这些已实测操作。

仓库范围检查通过（0 errors），工具自测33/33通过，结果记于 `zone-integration/validation.json`。`zone-integration/scripts` 是本轮脚本归档；复跑时复制到 `.agent-local/qa/TASK-100/` 并使用新的输出目录，不在证据归档中写测试存档。执行入口：`python -X utf8 .agent-local/qa/TASK-100/run_native.py <新的运行目录名>`；渲染脚本由显式指定本工程的 UEClient `runtime.launch_editor` 的 `-ExecutePythonScript` 参数启动。

## 仍需完成的整单门槛

1. 每区正式入口至旗点及撤退路线的完整输入实玩、战斗视线与掩体体验。16 栋局部穿行不覆盖整个区域。
2. 从新档连续完成 8 条主线，真实取得必要物资，不用传送／清兵／填事实跳过流程。
3. 逐条完成可获得的 15 条一次性支线，检验重复领取、人口与奖励唯一性。
4. 在正常夺回后使用第二营地，往返第一营地，保存、退出、继续，核对共享仓储及独立建筑。
5. Owner 对四区视觉和空间体验的验收，以及含新资产的 Shipping Cook／运行验证。

当前没有新增待定玩法决策。原有 TASK-087 模型／TASK-102 性能和整批真人、二机门槛不由本轮通过记录替代。

## 工程参考

采用 Epic 的 [FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine) 的独立 UCX 凸碰撞命名，以及 [Static Mesh Collision](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-collisions-with-static-meshes-in-unreal-engine) 的碰撞检查方式。导入通过 UEClient 公开 API，未绕过框架直接另写导入器。
