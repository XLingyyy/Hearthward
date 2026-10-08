# 石堡夜袭火烟接入

Owner 于 2026-10-08 当前会话批准首件视频并授权继续场景适配。

石堡 Actor 在序章、玩家距石堡原点 200 米内持有三个火点。相对位置为 `(-1400,2100)`、`(2800,2700)`、`(2850,4700)` 厘米，高度由原地形射线取得。每点包含一套火焰、一套烟、一个半径 550 厘米的暖色无阴影点光，以及三根无碰撞装饰木料。没有新增伤害、奖励、可交互对象或导航障碍。

组件由石堡 Actor 统一持有，重复 Tick 不创建第二组；离开序章或远离石堡即销毁，EndPlay 显式清理。原有 Campaign Restore/ResetActors 重建石堡，因此无需新存档字段或第二套读档回调。两个 Niagara 系统用原生类默认对象硬引用，Niagara 加入模块依赖；实际 Cook 加载仍待后续包验证。

## 验证

- UE 5.8.2 Development Editor 构建通过。
- `Hearthward.Hometown077.BedroomAndEscapeClearance`：1/1 Success，目标测试 0 warning、0 error。保留既有卧室、门口、回廊、后门净空检查，并新增重复启停、阶段变化、距离变化和 EndPlay 清理检查。隔离世界不自动注册控制器，测试显式注册；EndPlay 在该隔离夹具中显式调用。
- 正式自然地图通过原 UI `new` 创建隔离新档，再使用真实 `LoadPoint` 连续读档两次。每次当前石堡和全世界标记组件均为 6 个 Niagara、3 个火光；远离后均为 0；返回后均恢复 6/3。所有 9 根木料均为 NoCollision。
- 夜间实际游戏视口检查：庭院近景火焰、烟和地面暖光可见；后门方向能看到房屋外侧两个火点；回廊相机受原有实体栏墙遮挡，未将该画面记作火烟可见证据。截图在本地 `.agent-local/qa/TASK-096/fire-integration/frames/`，未通过修改曝光或诊断材质替代正式夜间画面。

原始报告见 `scene.json`、`native-index.json`、`native-result.json`、`build.json`。目标测试执行前的引擎启动日志还输出 13 条 `LogAutomationTest: Error: Condition failed`，以及驱动 TSR、MCP 和 SSGI 提示；这些不在目标测试条目内，原始过程日志完整保留，不声称整个进程零告警。首次构建的类型推导错误、两轮隔离夹具失败记录保留于本地，同轮最终结果如上。

当前证据覆盖火烟首件及接入生命周期。完整自然行走撤离、烟火附近实战/救援舒适度、白天整体材质、Cook 和新版性能验收仍未完成，TASK-096 保持 Active。
