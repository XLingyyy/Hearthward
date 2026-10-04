# TASK-053 · HUD紧凑亮面条槽测试（2026-10-03）

当前范围：按用户四项要求更新Development预览，未导入桌面端。Owner XLingyyy，分支codex/TASK-053-title-wheel，基线4db5789184fe38e041d62a1e68c8517338ea0b01；源码未提交／推送。参考图及来源指纹保存在reference.png／reference.json。

## 当前实现

- 三条生命／饱食／体力保持原红／黄／绿、前置名称和Gameplay实时比例／数值。条高从22增至24，行距从38收紧至30，因此条槽之间保留6个设计单位的细缝，各自有独立阴影；三槽整体高度从98减至84。
- 旧金属厚边采用上／下与左右端的分层反光，彩色条面有弧面明暗、柔和高光与左右切面；空槽仍保留完整框架。边框与高光随100%—150%字号等比例缩放。
- 弟弟信息的顶部从162移到129，随底槽计算14个设计单位的间隔；150%字号间隔相应为21。原弟弟进度、指令与新任务通知随同锚点移动，未修改其数据或行为。
- 同一入口[主界面UI测试.cmd](../../../../scripts/ui/主界面UI测试.cmd)进入GUID临时档池；启动进程CoreLimit=4，与近期加载预览一致，未改系统配置。

## 当前验证

Development Editor构建成功、无诊断／警告，17.22秒，完整结果[build-result.json](build-result.json)。执行命令：`python -X utf8 docs/qa/TASK-053/hud-gloss/build.py`，使用GameFactory UEClient公开API与UE5.8.2。

执行`python -X utf8 docs/qa/TASK-053/hud-gloss/launch_verify.py`，运行hud_gloss_aaa4251aae4e，**61/61检查通过**，含原有HUD原生51项与10项启动／夹具／渲染检查；没有改写或跳过原有测试。结果[report.json](hud_gloss_aaa4251aae4e/report.json)，原生详情[hud-preview.json](hud_gloss_aaa4251aae4e/hud-preview.json)。生成11张PNG：原生七张、真实自然地图满槽／部分消耗两张、原生部分消耗／近空槽两张。已目视核对四画幅（16:9、16:10、720p、超宽）、150%字号、四边反光、细缝与弟弟间隔。

实际自然地图部分消耗预览：

![实际HUD](hud_gloss_aaa4251aae4e/hud-gloss-partial.png)

原生透明HUD近空槽预览：

![近空槽](hud_gloss_aaa4251aae4e/hud_gloss_aaa4251aae4e-hud-gloss-empty-16x9.png)

[fingerprints.json](hud_gloss_aaa4251aae4e/fingerprints.json)绑定10项UI源码／原规则／布局配置／启动脚本与DLL，收尾核对当前字节一致。自有验证进程通过同一UEClient停止，开发GameUserSettings已恢复。使用独立GUID池，没有Shipping构建、桌面文件替换、正式玩家存档写入、Git提交或推送。

工具自测33/33通过。仓库自检0错误、git diff --check通过。正式基线范围检查仍为FAIL：基线缺TASK-053获批任务快照，不能将本机用户授权冒充基线审批；结果[repository-checks.json](repository-checks.json)。当前改动均在既有allowed_paths中。

## 保留记录与限制

首轮hud_gloss_97798c43821c未准备原有HUD套件所需的弟弟／Adventure夹具，导致12项与弟弟／新任务相关检查失败。源图、布局及失败日志保留在该目录；补齐与verify_title.py相同的显式夹具后，新运行61/61通过。首轮备份Markdown链接检查也失败，原文字节改以.md.original归档后重新检查0错误；基线快照限制仍保留。

自动化输入来自原生Widget与Controller事件，未替代Owner对光泽和复古观感的确认；仅本次HUD范围通过当前检查，其他页面此前PASS绑定各自历史DLL。
