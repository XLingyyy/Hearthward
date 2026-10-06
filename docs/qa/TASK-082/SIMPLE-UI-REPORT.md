# TASK-078 简约UI与储物箱更新

2026-10-06，Owner：XLingyyy，分支 `codex/map-1000m-heading`，构建时HEAD／基线 `ca21120cef4421d039c44248a0cfa5043c248feb`，受测未提交源码按下方清单绑定。后续GitHub提交／推送授权及源码一致性核验见[同步记录](GITHUB-SYNC.md)，原构建指纹保持不变。当前 Win64 Shipping 可玩包：`E:/AiAgent/XLingGame/Hearthward-Playable-20261006-SimpleUI/Windows`；含本地模型与运行库，无需UE或Python。

储物箱移除归火标志、宣传语、暖火背景和皮革面板，改为新版背包深灰三栏、分类、物品格、选中详情、装备属性与真实实例耐久、负重、数量和存入／取出。制作、记忆与约定、营地、田野与牧场同步换哑光深灰面板和简约操作。对话页去标志／中英文宣传语并居中；旧维修配置清理，维修入口继续进入新版行装管理。必要配方成本、状态、限制和取消说明保留。

存取继续调用已有StorageSubsystem原子转移，保留距离、容量和时间线校验。先前全屏矩形地图、地点与传送、红蓝三角方向火焰保留；没有变更主地图资产、玩法条件或保存格式。

| 当前检查 | 结果与证据 |
|---|---|
| Editor Development | 成功，[构建](simple-ui/editor-build.json) |
| Shipping Build/Cook/Stage/Archive | 成功，[结果](simple-ui/shipping-package.json)、[日志](simple-ui/package.log) |
| Standalone界面与存取 | 77/77 PASS，[结果／布局](simple-ui/runtime-report.json)、[Source／DLL指纹](simple-ui/source-manifest.json) |
| 仓储原子性／时间线与地图原生回归 | 4/4 PASS，2项保留LocalPlayer初始化警告，[索引](simple-ui/native-index.json) |
| 页面配置排查 | 18/18 PASS，[审计](simple-ui/page-style-audit.json) |
| 工具测试 | 33/33 PASS，[日志](simple-ui/tools-tests.log) |
| 仓库、范围、源／资源／程序绑定 | [最终检查](simple-ui/repository-checks.json)；严格基线范围检查缺TASK-078批准快照，失败保留 |

运行从实际新游戏进入，使用隔离GUID档池与受控库存、生产仓储访问组件和工作台。Slate点击分类、物品、数量与转移，木材从背包5／仓库0变3／2，再取出到4／1。数量不足、满背包、离开150cm访问范围及AdvanceTimeline后操作均被拒绝，库存保持不变；分页不改变库存。检查100%／125%／150%字号、720p、超宽屏和150%长页滚到底后背景仍覆盖视口。记忆页自动换行、底部提示溢出及田野缺布局字段已修复。18页配置无旧火红背景、logo或皮革面板；未使用的旧资源保留，改版标题标识保留。

![储物箱](simple-ui/storage-100.png)

![其他简约界面](simple-ui/ui-gallery.png)

更多实际截图：[150%装备详情](simple-ui/storage-gear-150.png)、[空箱](simple-ui/storage-empty.png)、[存入](simple-ui/storage-deposit.png)、[取出](simple-ui/storage-withdraw.png)、[容量拒绝](simple-ui/storage-capacity.png)、[数量拒绝](simple-ui/storage-insufficient.png)、[超宽屏](simple-ui/storage-150-ultrawide.png)、[大字体滚到底](simple-ui/camp-150-bottom-ultrawide.png)。中间夹具失败与修复见[过程摘要](simple-ui/intermediate-results.json)，原始失败日志保留于忽略的运行目录。

Shipping可执行文件SHA256：`ac76e39c914bf950f3af83f31f26c76ceea1b4e4d83f1de9d9fbc7536439f18f`。全部Source和UI打包指纹见[BUILD-INFO](simple-ui/BUILD-INFO.json)。独立Shipping启动6次窗口响应采样通过，[启动记录](simple-ui/startup-check.json)。ZIP全部152个成员CRC通过，[完整性](simple-ui/zip-integrity.json)、[交付记录](simple-ui/delivery.json)；大小4911468732字节，SHA256 `85caf3ec0a68dd21da83c2a37226a2433ee26a5da4e6906e7795bf80e7a1f7f0`。根目录正式／简约UI／地图／矩形地图启动入口均指向新包，旧包保留。

复验：`python -X utf8 scripts/ui/verify_simple_ui.py --build`，然后 `python -X utf8 scripts/ui/verify_simple_ui.py`；通过UEClient公开API。原生过滤器：`Hearthward.Inventory.Storage+Hearthward.Map078`。夹具仅在显式Development验证参数和GUID档池下运行，Shipping不包含。不是物理Windows键鼠、完整真人通关、长时间性能、第二机器或正式发行RC验收。此前地图98／旅行37／地理44项PASS仅为历史版本证据，见[火焰尖端报告](FLAME-TIPS-REPORT.md)，没有冒充本次完整复验。任务保持Active，不填Reviewer批准，不修改验证器。
