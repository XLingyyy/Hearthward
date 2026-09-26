# TASK-048 交接

2026-09-27，Owner／Reviewer XLingyyy；Issue可选。用户“继续048”，延续顺序任务设计→Owner确认→沿原编号施工的方式。canonical048对应原稿050；不自动把047设计批准扩展到048。

## 基线与范围

主干`e95fde185dc38d0b69c2de42ce66db0d1fd93294`已含047／PR49。范围登记基线`ed1a69a005eef1e371ae76accaac6b8aecf11a71`。分支codex/TASK-048-nature-content，独立目录G:/GameFactory/Hearthward/.agent-local/TASK-048；原TASK027及其他工作树保持原状。仅docs候选／设计与README，未下载LFS资产实体、未修改Source／Resources／Content／Config／Runtime。

## 交付

D1资源白名单及无工具起步；D2八野生三家畜四鱼与资产缺项；D3张力、饵料、2%随机与唯一奖励；D4三作物、照料及返种；D5喂养、两日繁殖、容量与断粮；D6持久化和权限边界。详见[设计](../design/DSGN-R15-nature-production.md)及[表册](../planning/TASK-048/TABLES.md)。所有新增规则待Owner确认，状态Blocked。

38项候选关系通过：20日有限采食，4人3点产320份、5人4点产400份；5人3点只有351份且断料。裸手来源可支付72木工作台与两件石工具。2%在连续理想成功下期望6—9事件/小时，无保底；一次清空4钓点96鱼期望1.92事件。两雄鹿外形只计一物种，野猪专用模型缺项；已有所有动物都未完成UE行为动画验收。

首次模拟按固定ID反复切源，与046实际优先余量最少点不一致；检查HearthwardCampState.cpp后修正模型，保留失败记录。没有修改游戏代码以迁就验算。结果与受检SHA见[报告](../qa/TASK-048/REPORT.md)。

## 下一步

请Owner确认或修改D1—D6。批准后扩展048实现范围，复用043／046／047接口，先做真实自然点与循环，再验证动物／钓鱼／种养／保存；不以本轮38项纸面检查代替UE验收。提交推送后核对远端并清理本单工作树；无需保留空闲目录等待审批。再次施工时从该分支恢复新工作树。
