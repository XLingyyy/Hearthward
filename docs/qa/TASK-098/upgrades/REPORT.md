# TASK-098 设施升级与生产火烟

2026-10-09，任务分支本地增量。原始范围基线 afc5b193；实现提交由后续 evidence.json 绑定。

- Blender MCP 原创 8 个升级附件网格和 1 个锻台火盆，3 个材质。二、三级附件按原 Facility.Level 累加显示；不修改等级条件、费用、岗位或保存格式。
- 冶炼、烹饪与锻造读取原设施/生产队列/劳动效率/批次状态控制炉火。暂停、缺料、无人、安全条件失效、出库堵塞时熄灭。篝火保持常燃；移动/升级暂停时熄灭。50 米外停用粒子和灯光。
- 复用已批准 TASK-096 火焰与烟雾，原 TASK-098 AlwaysCook 目录覆盖新包。所有新网格无运行时碰撞，原根碰撞保持。
- Editor 编译成功；Hearthward.Camp098.ProductionFire 原生测试 1/1 Success，0 warning/0 error。覆盖状态启停、四类三个等级、精确网格数和恢复无重复。该测试使用隔离状态夹具，未声称支付升级验收。
- 12 张逐级组合实际 PIE 渲染完成；此处保留四类三级图。场景为诊断平地与灯光，未修改正式地图。
- 篝火实际原接口支付 4 木/4 石并建造，GPU 下两个 Niagara 组件激活，保存/加载后唯一设施、注册和材料一致；18 项检查通过。图中的多方向灯警告来自诊断灯光，记录保留。
- 首轮 NullRHI 下 Niagara.IsActive 判定失败，原记录保留。改用同一状态驱动的灯光检查无渲染逻辑，并单独用 GPU PIE 验证粒子，未删去失败用例。

制作源：art_source/TASK-098/author_facility_upgrades.py、FacilityUpgrades.blend、facility-upgrades.json。原创新增几何，不含外部下载素材；沿用石木材质的来源仍按 TASK-096 登记。

完整流程实玩按用户要求后置。候选12已完成Shipping构建、实际容器34包核对和正常输入新游戏进入卧室。见 package-result.json、cook-assets.json 与 candidate12/observation.json。真实显示版本仍为20261009.3，文档已按屏幕更正，未据此更改可执行文件。启动脚本未做OS执行验收。正常鼠标/DPI、第二营地和 Owner 视觉签收不由本报告替代。
