# TASK-069 单项 Pause 量字测试提案

提案补丁：pause-font-measure-test-proposal.patch，仅 ExperienceTests.cpp 的一个测试与所需 include；未改 Root Source，未编译/运行。

自动化名：Hearthward.UI069.PauseAllObjectivesUseReadableNonOverlappingText。

依赖 Root 已授权的 DescribeLayout.components 四个诊断字段：font、tracking、fontRole、bind，值来自当前实际 FHearthwardUIElement；沿用现有 rect/text/visible/action/id。无需新 elements 数组或公共接口。ApplyLayout90–95会为动态按钮生成LayoutId，123加入LayoutOrder，所以现 components 可按 action 找到实际按钮。

复用既有 DangerousConfirmationNeedsExplicitFocus 的真实 Widget/viewport/角色组件 fixture；Comfort.TextScale在该 Instance 内设100，不写配置。HearthwardData::Rows("quests")提供当前全部34任务，每项真实设置 TrackedQuest、OpenPage(pause)、读实际DescribeLayout；正常与真实 ReceiveDamage 后的 Downed 各遍历一轮。

字体按当前主题 typography.body/display 路径构建同格式 FCompositeFont，选择逻辑、字号*.75取整、实际Tracking、整字符换行和1.6行步长均沿 NativePaint。每行由真实 Slate FontMeasure 取得宽高，计算所有已绘制文字的bbox；验证正文24下限、完整目标文字、声明高度、time/location标题和值与实际生存按钮不交叉。giveUp可点击点由其实际rect中心算出，不硬编码坐标。

测试输出实际遍历数及最大换行数，正常场景与OS路径仍由Root另验。未新增Inventory分类/Map探索两项测试；其绑定字号缺口只登记在前份报告。

编译/执行仍待Root；此提案不得记作PASS。
