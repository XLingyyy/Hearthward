Hearthward（归火）内部测试候选 0.2.0-preview.20261009.2
CandidateID: iteration-084-103-20261009-10

运行 Start-Hearthward-CPU.cmd 或 Start-Hearthward-Vulkan.cmd，保留完整 Windows 目录。无需 UE 或 Python。
本候选修复附近交互提示缺失：遗物、工作台、路标、采集和普通设施使用实际交互距离显示按键提示；离开范围隐藏；按键随设置变化。
验证：新游戏进入卧室，靠近遗物包观察底部“E 家中的遗物包”，走开后提示消失；返回并按 E 取遗物。之后按原任务继续验证。
两个入口共用本候选独立存档：%LOCALAPPDATA%/Hearthward/Candidates/iteration-084-103-20261009-10。没有迁移或覆盖候选9和编辑器存档。切换后端前关闭当前游戏。
WASD 移动，鼠标视角，E 交互，X 伙伴跟随，Z 等待，Tab 背包，J 日志，M 地图，T 交流，F6 存读档，Esc 暂停。
编辑器原生与 PIE 已检查此修复，Shipping 构建已完成；本包正常键鼠流程由用户继续验证。完整主支线、模型理解质量、联合帧性能、部分视听和来源、真人及二机仍未完成。本包未公开发布，未生成 ZIP。
请保留 Runtime/LocalAI、Resources、Content 与所有许可文件。构建来源见 BUILD-INFO.json。
