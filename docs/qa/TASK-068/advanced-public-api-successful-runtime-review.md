# TASK-068 真实模型自然活动补充用例

2026-10-05，Root 串行 UEClient，UE 5.8.2 / Windows 11 / Vulkan本地模型。HEAD `67fb0784ca8c6d488173e587e7f95c4be0d9092a` 加对应阶段未提交源码；锁定Qwen模型、两正例System、intent-first Schema、3328/256预算、并发1及冻结advanced输入/oracle。三例分别取得实际成功，未伪装成同一次整批PASS或全60理解验收。

| 用例 | 成功池 | 实际世界结果 |
|---|---|---|
| ADV-N01 浇水 | a6a07350-0bd8-45cb-a420-8c012a9326da | 公开 Nature.Act 实际种野菜；唯一目标 GUID 正确绑定，watered false→true，requested/acquired/delivered=1、carried=0、Completed，其余库存不变 |
| ADV-N02 新采石材返营 | 42c7f24a-3bfb-4738-aace-a75ce4792109 | 实际源 camp_048_loose_stones0 remaining8→7，营地stone0→1，弟弟最终不携货，requested/acquired/delivered=1、Completed |
| ADV-F01 钓鱼 | 33284f84-c86f-4731-b6dd-6b409e226c2c | fish_carp+1、bait-1、同rod GUID耐久40→39，鱼点stock24→23、successes0→1；普通成功反馈，没有额外奖励或其他物品变化 |

每例 fixture/raw/execution/E2E 均 true，恰好1次生成，未确认阶段无世界效果。N01/N02输入3009/3144，输出68/64，full_relevant、仅丢 own_bag_optional；Submit到UE结果22.750/20.469秒。F01输入3031/输出59、不丢上下文、8.234秒；这是单次暖响应，不能计为正式性能p95。F01真实稀有奖励分支没有执行。

## 修复与夹具边界

N01原实际 raw 正确却 TARGET_REQUIRED，经过单项 Native RED 后仅补齐 Interaction 两分支 braces，真实执行恢复。自然缺目标澄清独立 Native 回归仍保留。

N02先前采到一份但返营受实际灰盒西墙阻断，完整失败报告保留。只将 prototype Camp代理放到源同一侧 `(316.769,2856.405,30)`，源位置、stock、GUID、仓库/CampState、任务/oracle不改；公开地面trace命中临时ground，实际Capsule半高80cm/半径30cm，前后CampAt同为camp、半径5000cm。实际初距514.198cm，公共FindPath两点 valid=true/partial=false，执行中真实走到 `(268.483,2844.816,32.15)` 后入库，没有teleport返营或设置完成状态。

第一次修订夹具调用未反射的 GetCapsuleComponent，N02没有提交模型；随后改公开 get_component_by_class(CapsuleComponent)。第二次等待全局 nav build 结束超20秒，仍没有提交模型；改为等待实际本地完整path，保留路径有效性/非partial/5–8米前置。生产 Navigation 完全未改。池a6a07350与7eb1a7d6的前置失败不是模型理解错误。

所有成功与失败的 launch/results/stop 独立UUID保留，均 error=null并关闭相应Editor；聚合索引 `advanced-public-api-successful-cases-evidence.json` 仅挑出实际成功，失败证据仍可检查。临时地板/定位、测试营地代理及prototype SaveLoad均明确隔离。hunt/capture/camp_batch/escort、正常地图/菜单操作、Owner体验、全60+20矩阵和联合性能仍未验收。
