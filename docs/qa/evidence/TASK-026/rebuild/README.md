# TASK-026 Rebuild 本地证据

基线4114556；实施分支codex/TASK-026-natural-world-rebuild；未提交、推送或合并。

- `first-walk-425m.json`：第一版局部PIE行走。
- `sm5-movement-and-fords.json`：增密后主路线约425 m，两个约178 m跨岸测试；SM5，视觉不作为最终结果。
- `sm6-movement.json`：SM6主路线424.68 m/120秒，13项局部检查通过；完整路线为false。
- `views/`：最后一次材质和岩石贴地调整后的原始1600×900编辑器截图；全分区加载，仅作视觉检查，不作流送或性能证据。
- `assets.json` / `dressing.json`：原生资产与实例生成记录。
- `height-checks.json`：9个Landscape碰撞下射线采样。
- `foliage-refinement.json` / `grounded-rocks.json`：细叶采样与岩石足迹贴地调整。
- `locks-verified.json`：1408个新二进制路径远端锁覆盖，0缺失；其他lock文件保留中途失败及重试过程。

范围限制：PIE Slate统计包含编辑器与截图开销；不能解释为Standalone平均FPS/P95。完整11.264 km环线和两支路尚未完成实际行走；Owner视觉验收、完整Standalone跨区性能与独立评审仍未完成。

诊断纠正：早期rendering/surface报告查询的`grass.UseRuntimeGeneration`并非本机有效变量，返回0不能证明其实际状态；本机有效名为`grass.GrassMap.UseRuntimeGeneration`。独立模式实查默认为0，临时改成1未令草地出现，未将此开关作为修复写入启动入口。

## 最终局部修正复测

`final-local-walk.json`：35秒、124.98米、13项局部检查通过。`runtime-forest.png`为原始PIE截图，可见草簇。`water-rendering.json`记录19个水面网格关闭不兼容的Nanite。`grass-map-rebuild.json`只记录材质状态重新赋予与保存，未证明原生草图运行生成已修复。完整路线与Standalone性能仍未验收。
