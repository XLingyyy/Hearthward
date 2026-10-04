> 历史背包验证：下列结果仅绑定该轮源码；技能UI改造后的当前状态由 skills-preview/REPORT.md 记录，旧PASS不能代替重跑。

# TASK-053 背包UI测试版

按用户2026-10-02提供的厚实深色界面参考改造背包，延续当前HUD测试版。Owner XLingyyy，分支codex/TASK-053-title-wheel，真实基线4db5789184fe38e041d62a1e68c8517338ea0b01，唯一写入者。本轮仅修改本机UE 5.8.2 Development测试工程；桌面安装版仍为0.2.0-preview.20261002.2。未提交、推送、合并、公开发布或执行本次Shipping部署。

背包采用完整不透明视口与三个炭灰厚边面板，顶部保留原字体和金色选中光带，与标题、设置和存档风格一致。左侧为十个原装备槽及五列三行物品网格，中间为选中物品图像、类型、持有数量、原说明和实际属性，右侧为原角色资料。原图像资源复用，未使用参考游戏图像、图标或水印。

顶部只有“装备、材料、食物、工具”。移除任务分类入口，任务道具及图纸归入工具展示，保留原内部分类、关键物品限制、任务、藏宝图与图纸规则。实际装备、使用、丢弃及逐件行装管理仍调用原游戏动作；再次使用已装备物品仍按原规则卸下，换装仍需原0.4秒动作，不即时修改玩法。

属性来自原Gameplay、Inventory及gameplay.json。石制战斧显示原说明、攻击30、耐久80/80及单重3.20；其他物品只显示实际存在的攻击、防御、耐久、恢复、投掷或重量数据。角色资料仍为等级、经验、生命、饱食、体力、攻击、全身减伤、重击增伤、耐力消耗、负重及移动速度，没有引入参考游戏的智力、信仰、咒语、专注等属性。100%—150%字号均保留三栏，长说明按实际字体换行高度为属性区留出间隔。

## 启动和画面

双击同一[主界面UI测试.cmd](../../../../scripts/ui/主界面UI测试.cmd)，加载新游戏后按Tab打开背包，点击顶部分类和物品查看。滚轮／上一页／下一页翻阅，F装备或使用，R丢弃，Esc返回；逐件行装管理和设置入口保留。每次采用独立GUID测试档池，依赖本机已有UE、Python及GameFactory环境。

- [装备](../preview-inventory.png)、[材料空态](../preview-inventory-materials.png)、[食物](../preview-inventory-food.png)、[工具](../preview-inventory-tools.png)：实际新游戏的原库存与角色数据。
- [150%字号装备](../verify_fc1bdb7b8180/verify_fc1bdb7b8180-inventory-text-150.png)、[150%字号长说明](../verify_fc1bdb7b8180/verify_fc1bdb7b8180-inventory-tools-text-150.png)、[末页](../verify_fc1bdb7b8180/verify_fc1bdb7b8180-inventory-last-page.png)：显式非Shipping临时背包夹具。

四张场景图片直接截取真实PIE GameViewport；多页、空态、大字号与其他视口截图由真实原生Widget渲染，未合成或编辑图片。逐张观察见[视觉核对](visual-review.json)。十二张背包Widget截图的全部像素Alpha均为255，含720p和超宽，四角为黑色，见[不透明度核对](opacity-review.json)。小图标沿用原素材，大图展示的清晰度受原素材分辨率限制。

## 当前源码验证

- [inventory_build_r8](../inventory_build_r8/build_result.json)：UEClient公开API构建HearthwardEditor / Win64 / Development成功，无诊断。
- [verify_fc1bdb7b8180](../verify_fc1bdb7b8180/report.json)：308/308界面检查、56张原生UI截图通过。包括[背包80项](../verify_fc1bdb7b8180/inventory-preview.json)、真实PIE帧完成卸下／重新装备并同步槽图标、食物使用、丢弃、四分类、任务对象／图纸可访问、22件装备末页、空背包、模态输入、设置返回、原属性、150%字号、720p及超宽。背包临时库存、装备、Gameplay快照与反馈逐项恢复，原存档节点数量不变。标题、设置、存档、确认框26项和HUD51项继续回归通过。
- [hud_c797e0e8ec5c](../hud_c797e0e8ec5c/report.json)：真实自然地图32/32检查、十张场景截图，包含四个背包分类和返回HUD不重播旧任务。首次任务取样剩余4.6043秒，等待5.2秒后整组消失，J日志详情保留；[十米边界](../hud_c797e0e8ec5c/hint-range.json)11/11继续通过，涵盖三维边界、离开与返回、未知提示过滤，并核对位置与Gameplay快照还原。
- [hud_2175d7b05182](../hud_2175d7b05182/report.json)：实际Development -game测试入口自动加载自然地图，加载完成后三次连续窗口响应正常。自有测试进程经UEClient正常停止，开发用户设置原字节恢复。三轮23项配置／源码／DLL指纹均与当前最终源码一致，见[源码绑定](source-binding.json)；测试脚本的独立指纹记录在各运行目录。
- 仓库自检、33项工具测试、差异检查、正式基线检查与本地授权范围的独立结果见[最终验证](validation.json)及[范围核对](scope-audit.json)。本地核对确认其他页面配置、其他ScreenContent组合及原仓储页面未改。
- 八项桌面EXE／UI／正式玩家档／用户设置保持相同，见[保护结果](preservation-result.json)；安装版本信息也保持相同，见[版本核对](installed-version-preserved.json)。

正式基线范围检查仍因真实基线缺少TASK-053批准快照而失败，不作为PASS；本地授权范围审计不替代该检查。未通过未经授权的提交或更换基线绕过。

## 修正记录和验证边界

早期编译的指针类型、Map比较、局部变量遮蔽及窄化初始化问题已修正，记录保留于inventory_build_r1、r2及r6。r3为首版构建；r4结果记录脚本调用错误，未作为成功证据，r5公开API确认增量构建成功。r7完成长说明动态布局，r8为最终构建。

verify_1ce0881c9845发现大字号仍触发旧版纵向列表，已排除该回退。视觉观察进一步发现150%字号投掷火罐说明与属性重叠，已改为实际换行高度布局；verify_0a1a50404f4f的边界断言遇到浮点序列化差异，保留16单位间隔并只容许0.01单位数值误差。其他早期运行修正了验证对延时装备、原型场景未配置初始行装、Python反射及已装备物品切换卸下规则的处理，未修改这些原游戏规则。各失败原始记录保留，不计入最终PASS。

物理Windows键鼠完整实玩和Owner视觉分析仍待用户测试。本轮通过原生UI／输入管线、真实PIE帧及自然场景验证，没有当前HUD或背包的Shipping构建。前轮HUD证据见[HUD追加历史](../hud-refine/REPORT.md)，旧PASS只绑定各自源码。
