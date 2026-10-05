# TASK-069｜收尾全流程UI、设置和操作可访问性

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿071，阶段D，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-051、TASK-054、TASK-064、TASK-067。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

已有16页组件化UI、设置、加载／失败、兼容界面和051重映射。目标是实际全篇状态覆盖、中文输入、焦点、提示和显示比例，沿现有主题／组件修问题；不重新做全UI系统。

## 状态与操作矩阵

主菜单／新游戏／继续／兼容确认／退出；HUD生命、饥饿、敌人发现、倒地救援；地图发现与站点；23任务条件和领奖；库存实例、制作／维修、设施／岗位／口粮、钓鱼／耕地／个体养殖；弟弟Thinking／待确认／执行／受阻／完成／取消／旧结果，全流程每状态有明确入口／返回／失效反馈。

优先级沿051：加载／失败／确认→捕获改键／IME文本→菜单→钓鱼／建造动作→世界。中文组合输入Enter先处理IME，不误提交委托或触发E／R；切页、焦点丢失、读档和旧按钮回调不触发世界动作。重映射主要／次要＋一个修饰键、同上下文冲突拒绝、互斥上下文可用，保存设置与存档世界独立。

分辨率至少1280×720、1920×1080、1920×1200、3440×1440，并核对4K和Windows100／150%缩放；正文24px@1080支持100／125／150%，字幕32／40／48px与说话人／背景。重要信息不只用颜色区分。显示模式15秒确认／恢复，FOV90范围70—110、模糊默认关闭、震动及饥饿视觉可调；不删除物理减益。

## 暂停及验收

设置菜单默认暂停，弟弟对话运行，保存／加载／失败固定冻结。用户改暂停选项后每页使用同一语义，倒地期限、钓鱼和药品进度不按界面各算。音量／输出有实际设备试听记录；录音缺项继续标未产出。

复用现有布局工具，仅对改布局截图检查真实分辨率、长中文、最大缩放、空／满列表和按钮命中；真正键鼠跑新游戏→经营／对话→失败→读档→通关后菜单，并验证改键和IME。自动截图、物理输入、声音实听分开记录；不新增手柄导航、屏幕阅读器或未支持平台承诺。

## 建议施工范围

- `Source/Hearthward/UI/`
- `Source/Hearthward/Input/`
- `Source/Hearthward/Experience/HearthwardPlayerSettings.cpp`
- `Resources/UI/（仅实际布局／标签／状态）`
- `Source/Hearthward/Tests/（现有设置／输入定向用例）`
- `docs/qa/TASK-069/`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-004-presentation-gameplay](../../contracts/CT-004-presentation-gameplay.md)、[CT-TASK-051-input-traversal](../../contracts/CT-TASK-051-input-traversal.md)、[CT-003-save-load](../../contracts/CT-003-save-load.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
