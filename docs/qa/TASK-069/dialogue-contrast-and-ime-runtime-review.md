# TASK-069 交流可读性与真实 IME 局部验证

2026-10-05，Root 实际 Windows 键鼠，UE 5.8.2 / Windows 11 / D3D12 / RTX 4060 Laptop。HEAD `67fb0784ca8c6d488173e587e7f95c4be0d9092a` 加当前未提交施工。Editor Development 实际编译 PASS。

## 可读性 RED 与最小修复

原 pool8618582c 的真实亮灰地面上，交流正文、提示及按钮直接叠在世界表面，对比不足。只读调查确认 interface.dialogue 背景为空，三个组没有 asset，实际 layout 没有 background/surface；NativePaint 也排除了交流页通用深底，未发现图片加载失败。125%/150%文字重排还会删除空面板并将 choice 转 button，单补动态 choice 底色无法覆盖全文。

仅在已有 HearthwardScreenPaint.cpp 增加两行：dialogue 内容前以现有 Box/Color(panel) 填充 DesignSize。复用主题颜色 `15130FF5`，不改资源、输入框样式引用、布局或暂停规则。

独立 pool `7df06175-c2f6-4b27-a082-64e54b3af2b8`，PID44548。Root 实际按 T 打开交流，在1586×1026 Editor窗口的游戏视口中观察合成：深底覆盖实际设计区域，文字、按钮、正文和输入框清晰，世界仍在背景运行；设计区域上下留白沿原缩放行为。真实截图 [交流深底](dialogue-contrast-os-green.jpg) 已保存原像素，未使用 CaptureUI 透明离屏结果评估合成。仅当前实际窗口局部视觉 PASS，全分辨率/全部字号和 Owner 观感未验。

## 真实中文输入法

鼠标点 Draft，实际蓝色焦点框和 caret 可见。使用 sky press_key 的真实 n/i 键，Windows 输入法出现“ni”组合及“你／泥／妮…”候选；截图 [实际组合](dialogue-ime-composition.jpg)。实际 Space 提交候选“你”进入 Draft，然后再次 n/i 出现第二次组合。

实际 Return 只提交组合，候选窗消失，Draft 保留“你ni”；[回车后文本保留](dialogue-ime-enter-retained.jpg)。页面仍为 dialogue，generation_calls=0，server_process_id=0，没有委托候选或库存/请求变化。实际 e/r 进入输入法候选，没有观察到 E/R 世界交互；第一次 Escape 仅取消“er”组合并保留交流，第二次 Escape 才回 HUD。没有通过 type_text 或粘贴冒充本轮 IME。

21个夹具前置通过，350次采样、3个状态事件、28次布局变化。最终全部采样 authority/page/epoch/campaign/savepoints 精确等于 hold 初始 future，final_difference_from_future={}，零模型请求/进程，monitor error=null；Host CAPTURED、独占 PID 正常关闭。采样0.5秒，Draft/IME焦点不在journal字段中，以上操作信用由原生截图与真实键鼠顺序共同提供。

原始报告为本目录 dialogue-os-7df06175-*。临时地板、人物/物资定位、prototype存档与停用gameplay tick披露保留；该局部 IME 与底层修复没有覆盖正常新游戏、九页流程、右修饰键、声音设备、全分辨率或真人验收。
