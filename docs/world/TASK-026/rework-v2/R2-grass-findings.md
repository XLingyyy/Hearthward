# R2 草地诊断记录（未最终验收）

运行引擎 UE 5.8.2。原生 Landscape Grass 在编辑器、PIE 和新启动 Standalone 中实际可见。UE 当前实例存储下，Python 侧 GrassInstancedStaticMeshComponent 返回0不能代表渲染为空。

对照证据目录：`docs/qa/evidence/TASK-026/rework-v2/R2-editor-probe-02`、`R2-pie-initial`、`R2-standalone`。PIE已采集四组初始/返回图；Standalone已有G0、G1、G2、G2-return、G3。编辑器早期探测有重复采集，不冒充完整固定机位矩阵。Standalone返回矩阵仍需补齐。

G0关闭两类草，G1保留HISM关闭原生草，G2保留原生草关闭普通实例，G3两类可见。当前`ShowFlag.InstancedStaticMeshes=0`同时隐藏普通树石HISM，因此G0/G2背景变化是隔离工具的影响；对照不用于判断树石流送。

确认的缓存干扰：Standalone中`grass.FlushCache`会使GrassMap失效；在原生草关闭时调用，再开启后不能用该进程证明草恢复。该失败保留，不计通过。关键G2与返回采用新进程启动时即开启原生草；仅移动观察位置，不清空GrassMap。观察位置切换使用诊断传送，不能计入实走验收。

S1采用Landscape Grass为低草主层，HISM稀疏点缀。新增GT_S1_Meadow首轮密度500，固定视角确认覆盖不足后局部修订为3000（引擎实现单位为每100平方米）、Medium密度倍率0.4，XY尺度1.15—1.6、Z尺度0.25—0.45、消隐80—130m。新掩膜仅覆盖500×500m样段，20m边界渐变。原草层在样段外保留，母材质旧节点未删除。

草层保存曾因Standalone持有材质文件发生Windows共享冲突；关闭该进程后，仅保存计划内四个包成功。证据见`S1-grass-save-retry/apply.json`。尚需固定视角和新启动Standalone检视样段新密度，当前不宣称R2或S1视觉验收完成。

## 2026-09-22 新独立进程补验

S1草种最初仅存在于编辑器生成数据；新进程草图只有GT_Meadow。按原生ALandscapeProxy::PreSave流程，限定保存S1掩膜相交的4个地形代理包后，新进程GT_S1_Meadow的7个组件/110.26KB与编辑器一致。见S1-grass-maps-apply-01。

R2-standalone-matrix-03已完整采集G0—G3各自初始/离区返回，共8张1920×1080图。G2及G3-return人工复查确认原生草层可见。切换使用诊断传送，不能代替R4的分区卸载证明或R5实走。G2同时暴露远处草层硬消隐边缘，S1材质淡出需要继续修正。
