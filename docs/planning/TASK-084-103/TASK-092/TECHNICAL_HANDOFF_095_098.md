# TASK-092 给095—098的技术执行建议

2026-10-07。仅为源码／既有测试只读调查，未修改095—098任务文档。视觉首件待审见 [094具体决策](../../../assets/TASK-094/DOWNSTREAM_DECISIONS.md)，当前候选精确包见 [094映射](../../../assets/TASK-094/PACKAGES.json)。下面的调查、复现与现有规则下的技术修复可独立推进；未经运行复现的线索不写成Bug。

| 任务 | 可直接开工的技术入口 | 先复用的定向检查 | 应等待的部分 |
|---|---|---|---|
| 095 | HeroAnimInstance已有StoneAxeClipTime按真实Combat.Elapsed采样；BrotherAnimInstance已有实际Clips替换和180／360cm/s步频。先列ActionId、Clip、hand_r／Grip／BladeBase／BladeTip、权威有效时段，检查真实刀枪握点和动作过渡；保留当前速度和骨架 | `Hearthward.Equipment055.HeldAxeFollowsCurrentInstanceAndMode`、`StoneAxeLightUsesClipPhaseAndPhysicalBlade`、`StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade`已包含实际蒙皮姿态／武器Socket／接触时序，先运行并读真实失败位置；无需重建同样的石斧测试 | 新角色剪影／服装、专用射手／族人／盾／腿甲／箭袋制作源，Owner外观首件；新uasset准确范围／锁；正常连续输入和Owner动作片段 |
| 096 | HometownFortress原生结构和077三包立面分离。WorldPresentation已有LevelAddedToWorld订阅、Deinitialize移除、WorldClock ApplyTime；PresentationComponent已有OnSnapshotRestored／EndPlay退订。先审必经近景缺面、夜袭表现按阶段建销及Load去重；守住当前自然通路 | `Hearthward.Hometown077.BedroomAndEscapeClearance`继续作为平地结构夹具；其PASS不能替正式山坡导航。真实夜间／局部火光仍需同视角检查077Unlit立面和补面 | 门洞／楼梯／侧门碰撞替换等092实走边界；最终近景受光材质审样；不能借WorldPresentation提亮替代权威感知或恢复被拒整网格 |
| 097 | 先读094自然124包来源家族及14套R3/303动作精确链；实际AssetRegistry依赖和材质槽源配对优先于重导入。路线沿现有route_trace取加载分段，草／岩石检查包级碰撞和实例成本 | `Hearthward.Animals.FrameBoundaryAndEscape`和 `Hearthward.Farming062.IndividualGrowthAndProductProgress`保护动作／成长读取；`CropGeometryConsumesCalendarStage`已有作物几何阶段测试。仅在绑定错误被具体复现后修对应JSON指针或包 | 092路线净空／视线包络、096采用版本，野猪与作物首件；任何新增分区／HLOD／碰撞包。不要为配对不完整重做303动作 |
| 098 | CampaignWorld PresentLabor按真实Regions／Workers／Batch读取，现有设施生成与ID／GUID相联。先核对设施ID—工作位—交互中心—网格，以及interface图标键和图集UV。美术不得直接结算生产、改变footprint或把升级显示成建造 | `Hearthward.Camp059.WorkerAssignmentPresentation`已有表现刷新不推进日历／不消耗源的断言；`Hearthward.Farming062.CropProgressAndHarvestCapacity`、`PenProductiveAnimalAndPausePresentation`保护容量与暂停真值。由root协调093现有经营测试结果 | 093真值交接、097采用版本、设施与小尺寸图标首件；028共享包由098唯一写者，096仅消费；简约UI和原字体不被重打包 |

所有源码修改仍遵循各单实际allowed_paths与共享写窗口。表中引用的测试仅证明代码存在；本调查未重新运行它们，初始结果均NOT_RUN。复用测试通过后，只有新缺陷或接口变化才扩大范围。真实OS输入、近远景昼夜、Cook绑定和Owner审阅单列。

092目前只提供路线取证入口与实际胶囊／导航配置，尚未冻结新碰撞包络。与095—098的资产合同保持不变：不新增任务／敌人／资源／奖励，不改主地图或存档结构，不用表现事件再扣血、加物品或增长人口。
