# TASK-007 画面检查

检查对象：最终字号构建，原生HighResShot输出1280×720、1920×1080；逐张查看以下12张原图。

| 状态 | 720p | 1080p | 观察结果 |
|---|---|---|---|
| idle | [截图](1-idle.png) | [截图](2-idle.png) | 无进度面板，角色和场景可见 |
| running | [截图](1-running.png) | [截图](2-running.png) | 进行中、剩余4.0秒、约20%进度；中文可读，无裁切 |
| paused | [截图](1-paused.png) | [截图](2-paused.png) | 已暂停，进度与对应实际值一致；第一轮剩余3.8秒，第二轮4.0秒 |
| completed | [截图](1-completed.png) | [截图](2-completed.png) | 计时为5秒，面板隐藏，无虚假成功反馈 |
| interrupted | [截图](1-interrupted.png) | [截图](2-interrupted.png) | 已中断，橙色提示，无残留进度条 |
| expired | [截图](1-expired.png) | [截图](2-expired.png) | 提示到期消失，动作状态仍为Interrupted |

首轮字号过小的问题由HUD层字体缩放修复；没有改变场景、镜头、规则或图像内容。
720p面板约288像素宽，1080p约432像素宽，底部居中且位于画面内；标题、剩余秒数及进度条无重叠。
截图只证明所列状态与渲染尺寸；跨DPI、打包字体与正式领域动作仍未验证。
