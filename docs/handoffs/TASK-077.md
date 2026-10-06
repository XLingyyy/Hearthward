# TASK-077 山地石堡聚落

最新进展：用户要求完善开场并交付当前可玩Demo，已裁出第二版Marble的103680面立面并接到实心主堡，补背墙、基座和收边，材质按世界时钟调整昼夜Tint。原生4/4、撤离路线16/16及昼夜画面再次通过，Win64 Shipping打包成功，独立包已实际完成新游戏、背包、地图、跟随、手动存档和关闭重启后的恢复检查。发行版本0.2.0-preview.20261006.1，状态见 `docs/releases/demo-20261006/REPORT.md`。完整试导入模型仍隔离在 `.agent-local/qa/TASK-077/courtyard-v2-evaluation-content/`，运行Content只新增立面网格、材质和贴图。

2026-10-06，分支 `codex/TASK-077-hometown-stonehold`，基线 `53a1efd0ddf169a36265930aea63b58cabfaf061`。任务保持 Active；用户已明确授权本次成果提交、推送并集成 main。当前集成包含已验证的开场布局、运行资产、制作源及Demo发行脚本和证据；最终美术验收仍待完成。

当前游戏使用独立原生建筑 Actor：双床卧室、回廊、楼梯、庭院、侧门、城墙、角塔、主堡及六栋石屋。石屋增加坡屋顶、烟囱、门窗外观和灯笼；石材改用已有灰石资源，主堡增加扶壁。营地房屋类、四区数据、UI和存档格式未修改。

Marble高质量GLB已下载并试导入。材质缺少Nanite标记已通过公开绑定接口修复并持久化；完整贴图实机画面仍有严重碎裂、天空拉伸和缺面，原始GLB独立投影同样暴露几何问题。该模型未通过外观验收，已撤销运行时引用；uasset隔离到 `.agent-local/qa/TASK-077/rejected-content/`。原始下载、来源和失败截图保留，不再重复付费导出同一份结果。

框架公开 `UEClient.bindings.bind_pbr_material` 新增可选 `options.used_with_nanite`、`options.use_vertex_color` 和 `options.unlit`，均默认False。unlit将基色乘以Tint后连接自发光，使用无光照着色模型；实际引擎绑定通过。3个文件的补丁副本见 `art_source/TASK-077/ue-nanite-binding.patch`。游戏直接使用编译后的材质，运行不需要框架或Python。

最新Editor构建通过；新游戏、取护符、跟随、7段实际输入行走、兄弟到达撤离口和进入occupied阶段共16项通过。最终原生结果与全部证据索引见 `docs/qa/TASK-077/REPORT.md`。测试使用独立档池，未改动用户旧存档。

尚未完成：精细建筑美术、主堡及石屋内室、完整街巷生活细节、新增夜袭火光/烟雾/动画。当前为可运行布局，不能标记最终美术通过。任务基线无TASK-077快照，基线任务路径校验的前置条件不满足；未修改验证器。普通元数据检查另有记录。
