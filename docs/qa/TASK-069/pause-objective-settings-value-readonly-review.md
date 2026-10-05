# TASK-069 Pause 目标 / Settings 大字当前值只读复核

- 调查：audit_054；Owner / Reviewer：XLingyyy。
- 证据目录：`G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-069/`。
- 本报告只读 Root；未改 Source / Resources / Content，未运行 UE、build、Git。仅提出局部候选，未声明修复 PASS。

## Pause：确认事实

`normal-pause-red-os.jpg` 显示 main_01 三段目标换成五行，最后一行与“游戏时间”区域重叠。

- `HearthwardScreenContent.cpp:35` 直接读取 `quests[].objective`；无额外进度拼接。
- `interface.json:3412` 的 `pause.element.014` 原 rect=[1170,310,233,65]、Font17。
- `HearthwardScreenWidget.cpp:285–293` 先用空 text 创建 Element，随后 Resolve bind；`Element:262` 的24下限只检查创建时非空 text，所以此绑定仍是17。应在本次 Pause 资源中准确改为24；扩大所有 bind 的处理范围会影响其它页。
- `HearthwardScreenPaint.cpp:206–216,238` 使用实际字体和 Tracking 量字换行，行步长为 `E.Font*1.6`。非 HUD 不按声明高度裁断全文（222）。五行 Font17 的预留高度136，五行 Font24 为192；现65均不足。
- pause.info base=[1047,178,390,620]，右界1437、下界798。原 divider.1 Y383、time Y408/440、divider.2 Y489、location Y510/542。
- Widget317–321 的动态按钮为 menuPause=[1100,765,280,48]、giveUp=[1100,640,280,55]、cancelSurvival=[1100,705,280,55]。文字位置须独立避开这些区域，不删除功能入口。

实际 Resources/Data/gameplay.json 共34项任务，最长 objective 为 main_04：55字符，包含两个换行，三段长度17/20/16。main_08 为50字符，段长10/18/20；当前 main_01 为45字符，段长12/17/14。均为当前数据界限，字数不等同于实际 Slate 换行数。

## Pause：最小有限资源候选

保留现组件与按钮坐标，仅调整该信息块；字体24及宽332估计可令每个现有段落最多两行，必须由 Root 用实际 Typeface、Tracking 和同一换行算法确认全部目标≤6行。

| 对象 | 候选 rect / 位置 |
| --- | --- |
| pause.element.013 任务名 | [1138,272,290,40]，Font24 |
| pause.element.014 目标 | [1096,310,332,236]，Font24，正文角色 |
| pause.element.015 quest icon | [1092,268,34,36]，末304，避免遮挡宽目标首行 |
| pause.element.016 / .017 时间标题 / 值 | [1138,552,114,32] / [1256,552,172,37]；值 Font24 |
| pause.element.018 clock icon | [1092,550,34,33] |
| pause.element.019 / .020 位置标题 / 值 | [1138,598,114,32] / [1256,598,172,40] |
| pause.element.021 location icon | [1092,592,34,42]，末634 |
| pause.divider.1 / .2 | Y548 / Y592 |
| pause.mountains | 当前 [1078,568,321,183] 会落在新时间/位置背后；可局部等比缩到约[1201,638,198,113]，保留原装饰，按钮沿现有顺序后绘制 |

六行24的行步长预算230.4；目标声明高度236，末546。六行最后一行起点502、字身估计末533.2；时间/位置字身约583.2 / 629.2，图标末634，早于giveUp640。当前最长位置名“家中的遗物包”为6字，172宽预计可一行；标题四字用114宽。上述字身估计基于当前1.3系数，实际字形边界与换行须运行核对。

如果任何现目标在332宽实际超过6行，不得截断、压回17、限制字符或把时间移到640按钮。再采用 Pause 局部量字布局：复用现 FontMeasure 与 Paint 的换行算法，按实际总行数安排后续两条同行信息，并检验目标/信息底部早于giveUp起点；无需新增抽象。当前55字符数据宜先试有限资源方案。

## Settings：当前值消失的来源与最小修法

`normal-settings-150-value-red-os.jpg` 可见“字幕背景”“界面文字”行及左右箭头，但没有当前百分比。

- `HearthwardScreenSettings.cpp:269` 已正确生成界面文字当前值，304创建 Row.Value 文本；数据读取未缺失。
- generic readable 分支 `HearthwardScreenWidget.cpp:641–645` 给空 action 行寻找内部最长 label；654又移除所有按钮内文本子行。设置名称通常比“150%”长，因此只留下名称，当前值被移除。
- Root 当前 `ScreenSettings.cpp:290` 已加入 `ReadableLayout()?Row.Label+" · "+Row.Value:FString()`。这是最小修法：大字入口保留真实当前值，generic 无须再猜子 label，默认100仍采用原标签/值布局；settings.select/change/bind 入口保持原 action。
- 此次只读检查确认已合入该源语义，尚无修复后实际 OS 证据。

## 最窄运行门禁

1. 默认100正常 Pause：main_01 三段全文、时间、位置可读且互不重叠；确认24字号和目标完整。
2. 同一路径换最长 main_04（并检查全部34个实际目标的 Slate 换行上界）；若≤6行，再检查其目标与time/location/giveUp边界。不要以字符长度替代量字。
3. Downed / Busy 分别打开 Pause：giveUp与cancelSurvival未被正文挡住，菜单暂停仍可点击；原装饰未盖住正文。
4. 应用125/150后 Settings 辅助功能：字幕背景/界面文字显示真实值，点‹/›后当前值即时改变；应用后保留值。其键位页仍保留真实主/副键值和捕获入口。

本次未顺带修复的其他绑定下限缺口：inventory.element.008 category Font16、map.element.007 exploration Font17。Pause time Font22 已包含在上表局部候选；若 Root 仅先改 objective，time 下限也需登记未完成。

FontInfo Size 为点值；量字必须沿最终 FontInfo，不能凭 authored 数字比较像素。[Epic FontInfo](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/SlateCore/FSlateFontInfo)、[Epic FontMeasure](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/FSlateFontMeasure/Measure)。
