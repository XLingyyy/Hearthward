# 退出确认框返回按钮高亮修复

用户2026-10-02报告：鼠标未位于“返回”按钮时，该按钮仍反复亮起。保留现有登录、设置和存档视觉设计。

## 原因与修复

NativeOnMouseMove原先清除KeyboardFocus，但NativeTick每0.2秒Refresh又为确认框恢复默认cancel焦点；NativePaint直接把逻辑键盘焦点当作高亮，因此鼠标事件和刷新反复切换亮灭。

增加KeyboardNavigationActive，保留Enter的安全默认取消动作，绘制只显示真实悬停或当前键盘导航焦点。实际鼠标移动／点击切换到鼠标方式并清除可见键盘高亮；零位移的Slate鼠标事件不抢走键盘选择。刷新按动作重新定位悬停，避免元素索引改变导致旧悬停落到其他按钮。确认框内滚轮不改变选择或底层页面。

## 验证

- UEClient.build.project构建Editor Development成功：[最终构建](modal_focus_build_r3/build_result.json)。
- 最终真实PIE `verify_fb927cae9e15`：166/166检查通过，37张真实引擎UI截图；[总体报告](verify_fb927cae9e15/report.json)、[高亮回归](verify_fb927cae9e15/modal-focus.json)。
- 新增26项高亮检查：用实际NativeOnMouseButtonDown、NativeOnMouseMove、NativeOnMouseLeave、NativeOnMouseWheel和NativeOnKeyDown事件入口打开／操作退出确认框，检查与NativePaint共用的实际高亮判定。每种状态注入30次0.21秒NativeTick触发周期Refresh；未悬停、悬停返回、悬停确认、移开、离开窗口、键盘确认／返回、零位移鼠标事件、忽略滚轮、鼠标重新接管及点击空白均稳定。Enter安全取消、Esc及鼠标返回正常。
- CaptureUI新增可选PreserveFocus参数，默认行为兼容既有截图；以下截图保留真实焦点：[鼠标移开](verify_fb927cae9e15/verify_fb927cae9e15-quit-mouse-away.png)、[返回悬停](verify_fb927cae9e15/verify_fb927cae9e15-quit-cancel-hover.png)、[键盘确认](verify_fb927cae9e15/verify_fb927cae9e15-quit-keyboard-confirm.png)。已人工查看移开／悬停两张图，只有悬停图点亮返回。
- 工具自测33/33及仓库结构检查通过；范围审计无越界，20项最终源码／DLL／UI配置指纹一致。

回归夹具仅在非Shipping注册，要求明确verify_运行标识和PIE World；通过ScreenCapture引用inl编译，避免新增独立CPP被UBT旧源文件缓存漏载。首轮verify_50b2f4f260d7未加载命令，不能计PASS；verify_7360f2a92a10为最后3项输入边界修订前的历史证据。当前PASS只绑定verify_fb927cae9e15。

## 验证边界

这是Native Widget事件及引擎渲染检查，未注入物理Windows键鼠。Shipping的验证另检查构建、安装文件一致性和窗口启动，不宣称完成Shipping弹框的物理输入验收。源码位于codex/TASK-053-title-wheel，基线4db5789184fe38e041d62a1e68c8517338ea0b01加本地未提交修改；未提交、推送、合并或公开发布。

当前桌面修复版0.2.0-preview.20261002.2已部署并完成窗口启动、存档与用户设置核验，见[导入报告](desktop_20261002_focus/REPORT.md)。
