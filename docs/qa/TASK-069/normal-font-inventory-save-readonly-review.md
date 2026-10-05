# TASK-069 正常流程 UI 字号、底色与存档文案：只读审阅

2026-10-05。读取 Root `G:/GameFactory/Hearthward/.agent-local/task051` 当前源码和主题，实际核看 Root QA069 的 normal-title-os.jpg、normal-new-game-os.jpg、normal-inventory-red-os.jpg、normal-save-red-os.jpg、normal-pause-red-os.jpg。未修改 Root/Source/Content/Resources，未执行 UE、构建或 Git。本报告为根施工的有界建议，无修后 PASS 声明。

这些图片证实正常界面的视觉问题。运行来源、领物/跟随/保存/Continue/Quit 等操作结果由 Root 独立记录；本子任务的图片核看不代替这些操作证据或 Owner 验收。

## 已确认的字号与几何链

`HearthwardScreenWidget.cpp:248–265` 的 Element 对初始非空文字统一执行 `Max(Font,24)*TextScale`。`LoadElements:285–292` 后续设置 LayoutId/Component/FontRole，而 Rect 沿用主题的旧尺寸；ApplyLayout 只按组件几何比例变换，未按放大的字重新安排原生间距。ReadableLayout（565–566）只在 TextScale>100 且非 HUD/编辑/确认时运行。因此默认 100% 已改变文字尺寸，却继续使用原字号布局。

当前源码没有独立的 Font() 方法；实际链是 Element/LoadElements 的 E.Font，再由 Paint 构造 FSlateFontInfo。本文的 24 指既有 E.Font 约定值；Paint:199–200 再以 `.75` 转成 Slate 点值。Epic 明确 [FSlateFontInfo.Size 使用点值](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/SlateCore/FSlateFontInfo)，不能直接把该字段当物理屏幕像素。修正仍保留正文和功能文字的现有 24 下限。

| 实际可见问题 | 原主题 | 默认 100% 的消费与结果 |
|---|---|---|
| Title 英文页脚最后 RROW 到窗口底部 | title.element.014，font=9，rect=[112,872,580,20] | 抬到24；Paint:205–210 按宽度换行，非HUD的行数不按高度限制（216）；第二行 y=910.4，超出原20高框 |
| Pause 菜单与页脚英文多行 | pause.element.022，font=8，[720,772,330,40]；024，font=8，[55,891,560,20] | 同样抬到24，菜单英文额外折行；页脚第二行 y=929.4，只剩窗口底部少量空间 |
| Inventory 顶部两行装饰变成三行 | inventory.element.001，font=15，[248,39,215,65] | 抬到24，在旧215宽框内折出第三行，图片可见最后“的人”下垂 |
| Inventory 底部键帽/提示相挤 | 046 Esc 原font15、宽27；054 滚轮原font12、宽30；5组 label 原font16 | 全部抬24，但键帽宽高、label x 和组距仍为旧值；Paint:225–226 对键帽按实际文字宽度居中，超过框宽时从边框外起笔 |

底部键帽是功能提示，不能作为装饰缩小。装备 label 同样是功能文字：`ComposeInventory:117` 原 P−(5,28)、宽90，4字“主手武器”在当前24下限中容易折行。Root 提出的 P−(40,28)、宽160仍以 P.X+40 为中心，保持原槽位中心。

上述问题不支持全局移除 minimum，或把 ReadableLayout 的门槛直接改成 >=100。后一做法会让默认背包网格和全部其它页都进入现有单列流程，扩大行为和布局变化。

## 有限装饰角色与正文保留

Root 候选沿既有 fontRole 字段，把以下明确的纯装饰条目标记为 decorative，并由 LoadElements 复用该条目的 authored Font*TextScale：

- title.element.002、013、014、015、016、017。
- inventory.element.001、011。
- pause.element.022、023、024、025、026。

这些均为品牌/格言/装饰诗句，不含 action 或 bind。Pause 的“当前任务/游戏时间/当前位置”、Inventory 的属性分区、数值、装备标签、键帽、热键说明及 Save 的政策/安全文字继续保持24下限。不要按“font<18”或“无action”泛化豁免；这会同时缩小必要信息。

默认100保留现有布局。125/150沿既有 reflow 可再次抬到24，因为该分支会按最终字号分配新行高度。Root 已选择在 generic reflow:636 明确 `E.FontRole=body`：656 的测量使用 Typeface，Paint:199 也会选择同一 Typeface，避免空 FontRole 在30/36时隐式改选 DisplayTypeface。这是实际源码中的测量/绘制分歧；Map 在570–623的独立分支保持原已修测量路径。

## Inventory 两项125/150风险及最小修法

1. **重排内容无深底。** Theme 的 inventory.background 为空，Paint:23把inventory排除于通用黑底。默认100在现有 LayoutBounds 的 header/bag/stats/footer 填 panel、装备标签局部填深底，可以保留中心实时人物。125/150的内容已移动到x=190、宽1292；继续只给原四块区域填底会漏掉该列表。最小修正是在 inventory且ReadableLayout 为真时，于原底层 Layer用现有 Color(panel) 填 DesignSize，普通100保持四区域方案。该列表原本就不保留 hero image，无需新资产或额外 helper。
2. **功能热键文字丢失。** generic reflow:635跳过无action的keycap，随后只保留“返回/丢弃/装备/维修/翻页”标签。把字号和键帽几何修好，仍不能修复这个分支中的键名缺失。现有主题顺序固定为046键帽→047标签、048→049、050→051、052→053、054→055。仅在 inventory.footer 遇到 keycap 时暂存 Source.Text，在紧随的 text 行复制为 `键帽文字 + 空格 + 标签文字`，然后清空暂存值；放在既有635过滤之前即可。不需要新类、helper或改Source类型。必须消费当前 Source.Text，不能硬编码Esc/R/F/H，否则丢失重映射后的显示。5条完整说明沿既有可滚动文本流程展示；实际action行保持。

默认100的键帽、标签和5组间距继续按功能24放大。应使用实际最终字体和tracking测宽，或由真实渲染验证预定尺寸；不要靠缩字塞回旧27/30宽键帽。设备标签宽160方案可沿现有位置和局部深底执行。

## Save 为独立的重复占位

`HearthwardScreenContent.cpp:835` 动态安全文字位于 `[440,235,800,50]`；静态 save.element.003 政策说明位于 `[470,239,690,45]`。两段起点只差4，截图实证文字重叠。这条错误本身在原字号也存在，不能只归入字号问题。

Root 提出的最小局部修正可用：保留政策y=239/h=45，将普通安全/失败提示放到y=284；普通提示h=50、失败notice h=60；首行由302移到350，步距71、行高61和每页7行保持。政策框结束284，失败notice结束344，首行350有6的间隔；第7行结束837，仍在save.sheet的底部850以内。保留失败时禁用save、Epoch/确认、锁定/删除和50槽政策。更改范围为ComposeSave的展示位置，不修改保存逻辑。

125/150仍使用现有单列测高和滚动；不要依赖这条默认100坐标修改作为最大字号已通过的证据。

## 精确路径与最窄验证

Root 的施工候选只需要 `Source/Hearthward/UI/HearthwardScreenWidget.cpp`（LoadElements/generic reflow）、`HearthwardScreenContent.cpp`（装备label/ComposeSave）、`HearthwardScreenPaint.cpp`（库存现有底层和label填充）、`Resources/UI/interface.json`（上述有限fontRole/键帽几何）及QA069。Resources需由Root先在任务metadata精确登记；本子任务没有写入。

最窄工程验证可复用已登记的 ExperienceTests/现有真实Widget渲染方法：默认100的Title/Pause装饰、Inventory五组热键/主手label、Save政策/安全/首行三个矩形不相交；125/150确认body字族测绘一致、5条键名仍在可滚动全文、inventory列表有深底。再各核看发生修改的实际桌面页面；确认与正文24下限保持。无需重跑已经独立通过的HTTP/跨档全套，也不把CaptureUI当OS DPI或物理输入证据。

当前结论仅为修前源码与五张实际图片一致，Root候选经静态核对的有限建议。生产、编译、native及修后桌面结果由Root继续记录。
