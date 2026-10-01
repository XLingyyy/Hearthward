# UI 优化验收记录

需求来源为 Owner 提供的 `ui fix.docx` 和 `Hearthward/ui pic` 设置页参考图。本工作不使用任务编号，也不改变玩法、存档结构或原始参考文件。实施基线为 `main@9058ee2978f87cafa02e13666ebd0d96b81cd2f2`，工作分支为 `codex/ui-fix`。

标题页原先将 1672×941 背景与菜单一起放在等比适配画布内。16:10 等非设计比例下，画布之外透出三维场景。本分支让标题页与设置页的背景按原始宽高比居中裁切，铺满实际视口；菜单、按钮和点击区域仍使用等比适配的安全画布。图像不发生非等比拉伸，极端比例下会裁掉背景边缘。

设置页使用原有夜景和皮革面板资产，重排为标题、左侧八分类、右侧设置行、选中项说明与底部操作。自动保存、菜单暂停、显示模式、垂直同步、帧率上限、整体及分项画质、主音量、鼠标灵敏度和纵向反转是实际可调项；修改先留在页面草稿，点击“应用更改”才写入引擎或项目用户配置。R 将当前草稿填入默认值，Esc 返回时放弃未应用的改动。键位、辅助功能和教程页只显示现有操作说明；参考图中的按键重绑定、音轨独立音量和其他辅助开关没有对应系统，本次未绘制无效开关。

验证环境：Windows、UE 5.8.2、HearthwardEditor Development，Bootstrap 地图的真实 PIE。执行 `Build.bat HearthwardEditor Win64 Development -Project=G:/GameFactory/Hearthward-ui-fix/Hearthward.uproject -WaitMutex` 成功。运行同目录的 [`verify_ui.py`](verify_ui.py) 生成 `Saved/UIFix/report.json`，32/32 项通过：标题页 16:9、16:10、超宽截图，设置页 16:10 和全部八分类截图，自动保存间隔修改前未提交、应用后生效、恢复原值、返回标题页及 PIE 正常关闭。截图经过人工检查，无背景露边或文字重叠。

| 画面 | 截图 |
|---|---|
| 标题页 2560×1600 | [title-16x10.png](title-16x10.png) |
| 设置页“游戏” | [settings-game.png](settings-game.png) |
| 设置页“键位” | [settings-keys.png](settings-keys.png) |

尚未进行真人完整键鼠验收、音频设备实听、多显示器窗口模式验证或 Shipping 打包。此前 Demo 不包含此分支 UI 改动；需使用本分支重新编译的工程查看。

成果已归入 `Hearthward-ui-fix` 的游戏提交 `7eeecf52b4f2b1a72d7aa45d1f8d1e65e78a0486`。本报告的32项检查记录 fix2 之前的 UI 阶段，组合源码的当前验证及限制见 [fix2报告](../fix2/REPORT.md)。
