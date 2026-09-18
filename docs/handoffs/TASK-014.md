# TASK-014 交接

更新：2026-09-18。Owner：XLingyyy；分支 `codex/TASK-014-graybox-validation`。
基线：`d3d0fd9853b63d7c10fe4b678a6aaf3023d66667`。用户授权执行、提交和推送，未授权合并main；TASK-004未执行。

## 已实现

独立地图 `/Game/Hearthward/Tests/Graybox/L_GrayboxValidation`，资产目录 `/Game/Hearthward/Art/Graybox`。旧Backlog的Tribe预留目录按现有项目命名改为Hearthward，源代码、全局配置与Bootstrap主地图未改。
六个新资产包括：1张地图、1个100cm方块静态网格、4种恒定颜色/粗糙度材质。方块复制自UE5.8内置Cube，场景用原生Actor实例组装，不引入第三方下载、插件或生成模型。
场景具有60m×40m地面、5m刻度/30m标线、碰撞墙、坡道与200cm抬高平台、200cm净宽门洞、边界和地标。尺寸仅为PROTOTYPE_ONLY，R24和正式地图/攀越规则保持OPEN。
生成脚本只记录最初布局，发现已有保留资产即拒绝覆盖；后续修改以已保存UE资产为准。所有6项资产已取得当前账号LFS锁，保留至正常集成交接。

## 本机验证

- 原生UE Python通过UEClient启动/关闭编辑器；两轮PIE共28项检查通过，见[verification.json](../qa/evidence/TASK-014/verification.json)。
- 验证实际出生、直道行走、墙体阻挡、上坡到平台、门洞通行/侧墙阻挡、外边界、背包暂停/恢复、新PIE重新出生。没有自动生成伙伴、授予库存或启动模型。
- AssetRegistry扫描项目硬/软包引用，闭包为本单6个包，未引用Bootstrap或其他项目内容；引擎内置资源与Script依赖单独列出。
- 物理键盘由UEClient playtest经Windows SendInput执行，上坡/平台/门洞流程已录制。请求23秒、8fps，实际86帧，编码视频10.75秒；不能把编码时长当作游戏计时或帧率性能。
- 已检查3张1280×720截图与录像8个关键帧。首个冷启动录制帧存在材质/文字着色器临时显示，随后恢复；未将其描述为全程视觉无瑕或打包性能验证。
- 本单无C++修改，本工作目录复用013已构建的运行代码；独立GitHub源码克隆另行构建Editor并运行同一验证，已通过。

证据：[资产来源](../qa/evidence/TASK-014/asset-provenance.json)、[LFS锁](../qa/evidence/TASK-014/asset-locks.json)、[生成记录](../qa/evidence/TASK-014/creation.json)、[视觉检查](../qa/evidence/TASK-014/visual-review.json)、[录像](../qa/evidence/TASK-014/walkthrough.mp4)。
工程参考：[Epic关卡Blockout指导](https://dev.epicgames.com/documentation/unreal-engine/designer-01-project-setup-and-level-blockout-in-unreal-engine)，仅参考基础形体与尺度/通路验证方法，不引入外部游戏玩法。

## 验收边界

已从GitHub新建浅克隆，取回6个真实LFS资产，Development Editor源码构建通过（80.71秒），再次两轮PIE共28项检查全部通过。克隆未复制原工程Binaries/Intermediate/Saved，未下载GGUF；构建和验证前后Git工作树均干净。同机共用引擎及全局DDC，本机独立克隆不能冒充另一成员或第二台机器。
T-003要求的“A持锁、B受阻、合并后B接手”需要第二真人/账号，当前NOT_RUN；仅能确认真实远端锁已归当前Owner持有。Issue和独立评审未补齐，正式流程保持Blocked。
本单不提供正式人物/植物/建筑美术，不实现全地图导航、游泳、攀越或跳跃规则，不关闭R项。完整打包游戏验收NOT_RUN。
README和开发入口已同步；使用方法见[BUILD_AND_TEST](../qa/BUILD_AND_TEST.md)。

## 提交绑定与工作流检查

实现提交：`ebbcc922685755bf7a73d03c091e7d04a1f2b153`。原工作区PIE、截图和键盘录像对应该提交的资产与验证脚本；干净克隆直接检出该完整SHA构建和运行。后续提交仅补验证证据与状态文档，不改地图、资产或游戏代码。

[克隆报告](../qa/evidence/TASK-014/clean-clone-report.json)、[构建结果](../qa/evidence/TASK-014/clean-clone-build.json)、[克隆28项检查](../qa/evidence/TASK-014/clean-clone-verification.json)、[克隆截图](../qa/evidence/TASK-014/clean-clone-lane.png)、[工作流验证](../qa/evidence/TASK-014/workflow-validation.json)。

仓库结构检查通过，工具自测31/31通过。真实基线范围检查保留13项OUT_OF_SCOPE：基线014仍使用Tribe资产目录并缺少当前文档维护路径；未更换基线或绕过检查器。按用户本轮执行授权更新的任务范围另行核对通过，Source、Config、Plugins、Bootstrap、设计归档和004任务均未改。范围调整仍需后续正式评审。
