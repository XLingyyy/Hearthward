# 旧 Demo 营地存档迁移修复

基线：9058ee2978f87cafa02e13666ebd0d96b81cd2f2；本地修复分支 fix/legacy-camp-save，尚未提交或推送。

旧 Demo 使用 HWS2/schema3，营地保存在玩法 origin、campTier 和建筑列表中，没有 CampEconomy。升级链到 schema7→8 时一律要求该字段，导致整个档池读取失败；新游戏与继续游戏都读取档池，因此同时被阻断。

修复只允许原始 schema1—5 且营地经济字段缺失的存档沿既有 legacy Restore 路径恢复，迁移剧情营地位置，保留建筑、库存和存档点；不捏造历史建造费用。非空损坏字段以及 schema7 缺失经济数据仍拒绝。读取不覆盖原档，后续保存沿既有旧格式备份和原子替换路径。

验证：Editor Development 构建通过；Hearthward.Save 共8项原生测试通过（含真实用户档副本的2个存档点迁移、写入和重读）；仓库检查通过；工具33项通过；Win64 Shipping BuildCookRun成功，cook 0错误0警告。引擎初始化阶段已有 Condition failed 日志，目标8项测试均为Success。

桌面原目录已更新成修复版。运行测试使用独立 ShippingProfile 和原档副本；用户按 Escape 中止电脑操作，因此真实菜单点击和世界恢复的界面回归未完成，不声称已验证。

本地证据：Saved/LegacyCampFix/，含原档副本、Automation/index.json、build.json、tests.json、package.log。旧档不得提交到仓库。

修复源码 SHA256：7125839761ee7ad1470040c15e023278004b6ef56de44d77e4e4bbd78723ac5a
原用户档与备份一致：True
