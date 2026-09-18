# TASK-015 交接

日期：2026-09-18。Owner：XLingyyy。分支：codex/TASK-015-companion-ui。
真实基线：e447acb3cc7d01f9b2bcaead2432cf1506003c90。

## 实现

新增原生UMG交流面板，由现有HUD管理生命周期。沿用当前组件公共接口：Inventory读取负重，CompanionFixture读取阶段/真实交付和发送取消，LocalAISubsystem发送文字、取消推理、查询状态/台词。没有增加第二份库存、模型客户端或模拟运行数据。

开发模式临时T键在30米内打开，Enter发送，Esc关闭；输入模式切到UIOnly并平衡移动/视角忽略计数，关闭恢复GameOnly及先前鼠标显示状态。背包与对话互斥；对话本身不暂停世界。输入限制沿用013的1—1000字，空白/超长/忙碌/范围外不可发送。模型失败保留输入以便修正或重试。

进度按真实入库累计；受阻原因来自执行器，模型台词独立展示。完整台词在可滚动区换行显示。界面明确标记开发夹具/本机模型来源；关闭UI不自动取消013的待回请求，取消回复与取消委托分别调用已有接口。该行为为PROTOTYPE_ONLY，不关闭R20。全局建议尚无提供接口，显示不可用状态；不伪造三条建议，不声称Q257完整功能已完成。

## 验证与证据

- Development Editor最终构建通过，见[build.json](../qa/evidence/TASK-015/build.json)。
- 两轮PIE共31项检查通过：真实Qwen输出采集两份木材，执行器实际交付2/2；空输入/超长/忙碌、30米边界、思考、受阻、取消、旧时间线反馈、背包互斥、焦点和跨PIE清理均覆盖，见[verification.json](../qa/evidence/TASK-015/verification.json)。
- 使用Windows computer-use实际T打开、输入中文、Enter发送、Esc关闭；世界实际完成2/2交付，关闭后W移动从X=0到0.568897976381777。最终独立Editor -game窗口另检文字对比度和鼠标发送，见[physical-input.json](../qa/evidence/TASK-015/physical-input.json)。这属于本机运行，不是打包游戏验收；中文字符串输入通过不等于所有IME候选组合方式通过。
- 已检查思考、完成、缺料、过期、背包截图。首次焦点测试暴露不可聚焦根控件，已修复并复跑31项通过；截图又暴露EditableTextBox聚焦态文字颜色过浅，最终设置普通态/聚焦态颜色及18字号。31项状态检查在最后字体/颜色调整前运行，状态/输入绑定代码未再修改；最终构建与独立窗口检查覆盖样式变更。截图和覆盖边界见[visual-review.json](../qa/evidence/TASK-015/visual-review.json)、[最终输入](../qa/evidence/TASK-015/final-input.png)。
- 工具自测31/31通过（18.243秒）。真实基线路径检查保留13项OUT_OF_SCOPE，原因是旧Tribe预留路径、遗漏UI模块依赖及当前文档维护路径，见[base-scope.txt](../qa/evidence/TASK-015/base-scope.txt)。未改基线或检查器；按用户本轮授权更新后的任务范围另行核对，无越界路径。

## 设计和流程边界

未改主地图/资产、AI推理逻辑、伙伴执行器、库存或正式设计原件；004未执行。新增UMG/Slate/SlateCore为现有引擎模块，无外部依赖安装。
Issue、独立评审与正式prototype审批未补齐，流程保持Blocked。没有合并main、打包或第二台机器验收。
沿用独立项目现有代码与证据路径；GameFactory UI技能中的浏览器交付、generated_ui产物和专用pipeline角色要求不适用此次现有原生UE项目维护，未新增浏览器层或框架改动。开发和运行验证均由用户本轮授权执行。

参考：[Epic UEditableTextBox](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UEditableTextBox)，以本机UE5.8头文件和实际编译/操作为准。
