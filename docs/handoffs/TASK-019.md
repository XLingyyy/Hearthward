# TASK-019｜营地仓储数量选择与双向存取

用户2026-09-19授权自行推进、提交和推送。任务分支 `codex/TASK-019-storage-menu`，基线 `fe13f0bf2bc10a732a7098aa08dfb87a46e96ed7`。本单实现验证已完成，提交绑定随后记录；未合并main。正式流程仍Blocked：Issue查询401、独立评审和prototype正式批准缺项。

## 当前行为

显式执行 `Hearthward.Companion.CreateTest`，在现有营地150cm内按R打开仓储。五种物品来自现有定义，显示随身/仓储数量、单重、负重/余量及所选物品最多可取数量。选择物品并输入正整数后点击存入或取出；实际结算复用同一共享仓储原子转移，不创建物资。容量不足、库存不足、无效数量、离开营地、营地销毁或epoch过期均不转移。

查看默认暂停；Esc/R返回，Tab切背包，F6切存档，与伙伴对话互斥。仅释放自己取得的暂停与输入限制，保留已有外部暂停。加载旧档关闭仓储会话，旧Widget无法对新会话提交。018的E五秒木材入库仍保留；本单R即时确认存取、150cm和临时键位均为PROTOTYPE_ONLY，不补定正式营地解锁或R23。

## 最终验证

- [Editor构建](../qa/evidence/TASK-019/build.json)：UE 5.8.1 / Development Editor，最终源码PASS。
- [两轮PIE](../qa/evidence/TASK-019/verification.json)：49/49通过，包含五物品双向守恒、数量边界/容量拒绝、每次点击距离复核、旧epoch、已销毁营地、外部暂停保留、菜单互斥、回档撤销旧界面及第二轮PIE磁盘恢复。
- 五类下拉选项切换后逐次强制垃圾回收并继续渲染/存取，覆盖实际发现的生命周期回归。
- [最终实键录像](../qa/evidence/TASK-019/physical-final.mp4)：R开仓储、鼠标选矿石、键盘输入2、实际存入/取出两份矿石、Tab背包、R返回仓储、F6存档、Esc返回和S后退；屏幕显示负重13.70→9.70→13.70。观察范围及录制元数据见visual-review.json和recording.json。
- 仓库结构0错误、工具31/31。基线范围检查如实失败：基线没有本单任务快照；当前授权范围无越界，见workflow-validation.json。

运行发生在提交前工作树；最终源码字节归档tested-source.zip，覆盖见working-tree-binding.json。pre-gc-fix-physical-verification.json只保留早期定向结果，不作为最终验收。

## 实际修正与限制

输入框普通/焦点文字及下拉项默认颜色对比不足，设置明确样式。新增下拉项首次实测出现Slate访问异常；UE的HandleGenerateWidget仅保留Slate结果，自定义UWidget未由根WidgetTree持有。增加菜单生命周期内的UPROPERTY引用，随后完整49项与实键鼠通过。保留ui-crash-excerpt.txt、ui-fixes.json，未将崩溃隐藏为成功。

未新增依赖、资产或存档schema；正式营地/完整物品目录/兄弟转交/建造配方仍待后续。未重新跑真实模型、打包或第二台机器。本单复用原生库存接口，验证集中在实际UI/PIE，不新增重复的库存单测。TASK-004未执行。
