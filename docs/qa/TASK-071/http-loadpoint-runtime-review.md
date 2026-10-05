# TASK071 实际HTTP跨LoadPoint定向运行

2026-10-04。Root已运行第三版公开Goal/任务量前置的真实PIE脚本；子代理只读取既有结果形成此QA，未运行UE/build/model/HTTP/Git，未改Source/Content。

原始结果：`Saved/Task071/http-actual-http-loadpoint-semantic-baseline-3a5bc510-3123-4200-9c7e-5c98c1997a69/results.json`。ok=true、stage=done、674项check全部通过、0 fail；旧实际HTTP dispatch后观察125.438秒。launch成功，Root持有并关闭Editor进程45752，stop.ok=true。未检查日志warning总数，不能把674检查0 fail写成全进程0 warning。

## 实际前置与结果

- 隔离pool：3a5bc510-3123-4200-9c7e-5c98c1997a69。复用TASK068公开prototype夹具的真实Ground、单Companion/camp/workbench、正常背包加料/共享仓储转移、实际baseline SavePoint；敌人定位在camp外，prototype source_safe输入明示。没有正常人类游玩或模型修改world权限的信用。
- baseline实际Phase=CANCELLED，GetGoal intent/item=None、quantity0、mode/source为空；Requested/Delivered/Acquired/Carried全部0。源码正常wait→Cancel会形成该Phase，当前语义前置准确通过，未强设Idle/清Command。
- 第一轮C01实际模型返回匹配collect/wood/1/additional_acquired/S1、limits/unresolved空的真实候选；未确认时世界物资不变。实际LoadPoint更换epoch并清busy/candidate/raw；旧真实CandidateId确认返回false及STALE_CONFIRMATION，库存仍与baseline一致。
- 第二轮C02实际生成HTTP前置满足：generation_calls1、input_tokens3233、full_relevant、dropped=[]、status弟弟正在思考、raw为空。观察到这一前置的同一Slate回调实际LoadPoint；busy/candidate/raw清除，近端观察无旧回复、候选或库存变化。
- 第三轮fresh C01实际模型候选GUID与旧ID不同，未确认不影响库存；在新候选旁旧ID仍不能确认，新ID可正常确认。真实执行完成，camp wood精确+1、来源wood精确-1、Brother wood不增，玩家背包不变。正常SavePoint公开checkpoint为Completed、Requested1、Delivered1、Acquired1、Carried0。
- 持续观察至旧dispatch后125.438秒，最终库存/任务量无额外变化；再次真实SavePoint的公开任务量保持上述完成状态。整个流程没有固定response、mock HTTP、手动调用旧handler或生产private状态访问。

## 历史失败保留

1. `Saved/Task071/http-actual-http-loadpoint-reboot-7c2d77fa-723c-495b-8f63-8702561881d4/results.json`：fixture阶段protected World读取失败，0实际model请求，进程已关闭。
2. `Saved/Task071/http-actual-http-loadpoint-public-api-4399ff10-9fd2-4b8c-abb5-b20a85781228/results.json`：fixture阶段错误要求baseline Idle，0实际model请求，进程已关闭。

两次都不是模型/LoadPoint业务RED。第三版删去World/NPCReceipts/NPCOperations/CommandActive保护字段读取，先记录实际公开Goal/任务量/Phase再判断。没有增加生产getter、反射私有schema helper或用setter凑前置。

## 信用边界

本次实际通过的是进行中HTTP跨Load真实取消／结果隔离、旧候选确认失效、零额外物资结算及新真实请求完整执行。真正迟到的成功200 response是否进入ExpectedSerial handler没有公开观测，本结果forced_late_successful_response=NOTRUN；不能把125秒quiet观察冒称强制触发了该分支。

npc_receipt_operation_checks=NOTRUN: no public Python API。原生Save13的receipt幂等与版本诊断结果独立，不记为本次Python读取结果。human_menu_physical_input=NOTRUN；普通标题菜单加载、真实键鼠、旧Slate控件callback仍待独立验证。该三轮不是068完整60模型门、072性能门或Owner最终验收。
