# 编辑器独立游戏窗口修正（2026-10-08）

用户报告：从UE编辑器独立进程运行时窗口超出屏幕，无法调整。

## 复现

在未修改偏好的编辑器中通过真实运行菜单启动独立游戏，实际参数为 -game -PIEVIACONSOLE -windowed -WinX=0 -WinY=51 -ResX=2551 -ResY=1468。本次没有 -ForceRes。窗口捕获外框2560×1600，原点(-1,-47)，顶部标题栏不可见。编辑器保留NewWindowWidth=2551、NewWindowHeight=1468、LastSize同值，未启用CenterNewWindow。

## 改动

项目DefaultEditorPerProjectUserSettings.ini与本机已有偏好统一为1280×720、CenterNewWindow=True、NewWindowPosition=(-1,-1)。编辑器通过拥有进程的UEClient正常关闭后修改并重开。引擎PlayLevel.cpp在CenterNewWindow=True时不会用退出窗口LastSize覆盖NewWindowWidth/Height，避免预览尺寸再次积累。未修改引擎源码、游戏渲染分辨率和玩家存档。

## 验证边界

- 重启后通过官方MCP读取实际LevelEditorPlaySettings默认对象，四项值正确。
- 通过公开UEClient在独立测试UserDir启动同等窗口参数的Development游戏，实际外框1922×1128、原点(319,177)，完整可见，标题栏与关闭按钮可见；系统缩放后的客户区为1920×1080。
- 独立测试通过其UEClient正常关闭；重开的编辑器保留运行。
- 后续实际Computer Use从编辑器播放菜单选择独立进程游戏，确认真实子进程参数为-windowed -ResX=1280 -ResY=720，外框1922×1128、原点(319,177)，完整位于2560×1600屏幕内。实际鼠标拖动右下角缩至1612×954，进入新游戏后HUD和暂停菜单完整可见；Alt+F4正常退出后窗口消失。见[菜单实测](window-menu-20261008/result.json)。
- 退出后的第二次菜单启动未完成：工具报failed to activate captured window，重新选择窗口并重试一次仍失败，按Computer Use恢复规则停止UI操作。本项不影响已完成的首轮菜单启动/缩放证据，重开信用仍未取得。
- 配置改动无需C++重编译；完整游戏流程未在此窗口测试中重复执行。

私有原始记录：.agent-local/qa/TASK-084/window-20261008/，编辑器所有者记录.agent-local/qa/MCP/window-bug-editor-20261008-02/。
