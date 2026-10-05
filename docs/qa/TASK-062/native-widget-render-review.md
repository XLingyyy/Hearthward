# TASK-062 原生 Widget 渲染核看

2026-10-04，受测目录 `.agent-local/task051`，基线67fb0784ca8c6d488173e587e7f95c4be0d9092a上的未提交生产补丁。

实际命令由 TASK-053/run_native.py --render --extra-arg=-Farming062Render 调用 UEClient；原始报告 Saved/Task053/render062-baseline069modifier/index.json。三个 Farming062 用例全部通过；同次069修饰键真实RED单列，整份combined报告不称PASS。

FWidgetRenderer 将实际 HearthwardScreenWidget 绘入1696×954 RGBA8。五张源图和布局位于 Saved/Task062/native-widget-offscreen：crop-half、crop-mature-full-bag、juvenile-quarter、goat-fed-pen、pig-pen。根已逐张核看中文、3行状态、所有实际操作项与页面边界，未见文字/控件重叠。半成熟50%/1440W分钟、成熟100%/0且满包保留收获、幼年25%/2160、山羊奶6/24与剩余1200、猪栏无蛋奶信息均呈现真实fixture状态。

方法和边界见同目录method.json；这是明确C++夹具的离屏实际Widget渲染，未作为PIE、桌面键鼠或正常新游戏验收。首次fixture遗漏Presentation组件的崩溃保留历史；后续补齐真实组件，生产不加入掩盖夹具错误的fallback。
