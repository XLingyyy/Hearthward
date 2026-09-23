# S1 样段审阅记录

状态：**审阅准备中，Owner未批准，026整单未完成。** UE 5.8.2；4032m地图中的500×500m样段；自然场景先行，无人文设施。

[打开六机位前后对照](S1-review.html)。原生图片均1920×1080，六机位位置与朝向固定；Before为`R0-editor-before`，After为`S1-editor-after-05`。截图没有合成处理。

## 样段变化

- 营地核心保留空地，林缘增加高低层次；42棵island_tree_02提供第二实际树形。
- 林下土色、草层空隙和树根过渡；Landscape Grass承担低草主层，独立草材质提供距离淡出。
- 三组不同尺度岸石与低灌木绕开通路；第二轮新增60块小石、35组灌木。
- 两层不同方向/尺度的细波减弱规则条纹；共享材质使用S1掩膜限定，未推广全图。
- Generated批次按计划更新，Authored树桩保持；资产/外部包核验本人LFS锁，保存前备份，仅保存计划包。

## 操作与证据

从GameFactory根目录运行：

```powershell
.venv/Scripts/python.exe -X utf8 Hearthward/scripts/world/TASK-026/open_rebuild.py --game --medium
```

进入`/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds`；WASD与鼠标操作。默认350cm/s，无伙伴、仓储初始化或本地模型进程。角色仍使用已有灰色胶囊显示，026不更换角色资产。

下列证据相对`docs/qa/evidence/TASK-026/rework-v2`：

| 项目 | 证据 | 当前结论 |
|---|---|---|
| 六固定视角 | R0-editor-before、S1-editor-after-05 | 已采集并人工复查 |
| 地面/胶囊 | S1-ground-checks-03 | 最新岸石布局30个不同位置全部通过 |
| 正向Standalone | S1-standalone-forward-02 | 全部324点通过，约740.7m/208.8秒，无路线传送 |
| 正向初步成本 | S1-standalone-forward-02/cost-summary.json | 1080p Medium，67.28FPS，P95 18.61ms；细波修改前 |
| 反向Standalone | S1-standalone-reverse-02 | 最新布局收束中；reverse-01为较早布局通过证据 |
| 草层隔离 | R2-editor-matrix-03、R2-pie-initial、R2-standalone-matrix-03 | 各G0—G3初始/返回均有图，修订时间不同 |
| 草图持久化 | S1-grass-maps-apply-01 | 4代理保存，新Standalone确认7个S1草图组件 |

原始CSV保留全部帧，不删长帧。以上不替代R6全图性能，隔离图的诊断传送不计入正常步行。

## 遗留项

- 远山大面积灰坡过于光滑，局部石群仍显机械；背景轮廓和地标待全图地形/山地精修。
- 湖水深浅与岸线偏简单，当前只完成局部波纹修订；水岸几何和全河段接缝未正式收束。
- 开阔地仍能看到草卡片重复，营地外缘部分视角较空；调整时需保留核心空地与两条出口。
- 全图五类自然区尚未逐区精修；完整11.264km主环线、支路、双跨岸、HLOD和真实卸载未验收。
- R1高度补丁/水段增量、R6正式性能、Reviewer/Issue、第二成员干净克隆均未完成。

## 审阅范围

Owner判断样段的树形组合、林下地表、草层和岸边密度能否作为后续分区的质量方向。远山、水岸及上述缺项仍继续返工，样段认可不等同于整图验收。

依据[SPEC第12.1节](SPEC.md#s12)，只有明确认可“可作为全图质量样板”才推广。未收到认可前可继续修样段与依赖分析，不能自动填写Owner批准。
