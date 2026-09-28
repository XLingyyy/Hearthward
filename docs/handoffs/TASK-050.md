# TASK-050｜实施交接

Owner／Reviewer XLingyyy；无Issue。真实050＝原稿052。用户已确认D1—D6并授权施工、提交和推送；本分支不合并main。基线为已含049的 `main@3a163b919ad9e14d987f0197c491c58c890dcb71`。D1的EquipmentId维修需修改Building服务，任务范围漏列该路径；已在 `eab1387b33121796eef922ff2446d892d561b5b6` 补录。首批受测代码提交为 `f008e19d48bf61ae99cd878a0c37e515097753d8`；后续补上营地资源点委托和玩家当面交付入库。复用 `G:/GameFactory/Hearthward-task049` 工作树，原 `G:/GameFactory/Hearthward` 的027改动未触及。

实现沿029同一能力目录、任务卡、弟弟执行器、Nature结算、记忆与Save入口扩展，接入受限已有货物入库、已知地块／栏舍照料、指定装备实例维修、规则槽位、一张卡取消所有任务和约定，以及W三日回营交流。新进度及首次会合前存档从首次30米会合起算，旧档缺字段从载入W起算。救援抢占暂停消耗性活动，玩家明确继续后恢复。044自动食药与救援底层规则继续沿用。

UE 5.8.2 Editor Development构建、NPCAgent原生15/15、增量PIE 26/26、原050完整PIE回归108/108与首次会合PIE 17/17均通过；范围/L0使用 `validate_repo.py --task TASK-050 --base eab1387`。具体证据、夹具、未注册能力与NOT_RUN见[报告](../qa/TASK-050/REPORT.md)和[能力矩阵](../planning/TASK-050/CAPABILITIES.md)。存档仍使用schema8，新增记忆字段走已有存档结构；真实旧schema8文件迁移、模型输出、自然地图任意路径、长程角色体验和Shipping未在本单验证。

050仍未注册仓库→玩家及其他容器搬运、营地外安全点采集、栏舍普通产物、专项猎捕／钓鱼／牵引／救援者护送及限定批次在岗生产委托。栏舍物品与产出规则缺失，且任务非目标禁止修改运行数据表；钓鱼、牵引和救援者移动绑定玩家，生产没有委托批次回执。扩到这些活动需要先确定领域规则与本单范围，再逐项接唯一领域结算和自然场景验证。验收本分支请打开 `G:/GameFactory/Hearthward-task049/Hearthward.uproject`；原目录 `G:/GameFactory/Hearthward/Hearthward.uproject` 仍处于另一个检出状态。任务保持Active待Owner验收，不创建Issue或自批合并。
