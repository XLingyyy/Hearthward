# TASK-070 两件样板的实际材质与碰撞修正

2026-10-04根，UE5.8.2，未提交分支 codex/TASK-053-traversal/base 67fb0784ca8c6d488173e587e7f95c4be0d9092a。两件使用现有真实FBX及四张4096²源贴图，通过公开导入API得到各6资产，精确路径和Owner XLingyyy服务端LFS锁记录已登记；新增两件本地PBR材质亦已登记和核锁。

真实导入RED：equipment-import-red-probe.json。两MIC使用共享FBXLegacyPhongSurfaceMaterial；Roughness接ShininessMap而转换，Metallic贴图未绑定，R/M仍sRGB默认色彩；各mesh有1凸包。首次异步加载显示32px为未完成纹理编译占位，后续完成加载后的实际size与registry source tag均4096，未当损坏重新导入。

公开API修正已实际执行成功：pbr-results.json，两个新本地材质使用Color RGB→BaseColor、Normal RGB→Normal、Roughness R→Roughness、Metallic R→Metallic；R/M线性TC_MASKS。材质compile_errors[]，mesh slot0绑定新材质，simple/convex均0。仅保存两个材质、四个R/M纹理和两个mesh，共8资产；旧MIC/共享父材质未编辑。

独立新Editor读盘结果：equipment-import-probe.json，两个piece无read_errors，全部read_complete。实际mesh slot0和material graph四路已重读；R/M sRGB=false、TC_MASKS；纹理4096²；无asset hull。真实SceneCapture夹具中组件NoCollision读回0，未将资产hull0替代组件碰撞事实。

根已实际核看 import-red-calibrated 与 pbr-fixed-calibrated 的两件oblique PNG，保留在同名QA子目录。两组使用同光照/camera/PPS，石矛木柄与石刃、短刀皮革握柄与刃面纹理清晰，修复后粗糙度/金属度按原纹理生效。每组完整四视图和实际UE-local顶点仍在Saved/Task070对应weapon-render目录；capture_complete=true，无asset_mutations，world未保存。曝光黑图、白剪切及physical EV10仍白的失败记录保留，摄影理论值未冒充实际曝光测量；当前30/10lux仅固定可视fixture，详见fixed-exposure-evidence.md。

这只覆盖两件独立样板工程接线/碰撞/真实渲染。D04人物/建筑/地表三组样图的Owner视觉验收、第三类实际样板、实际握点/长度/动作及正式装备绑定、LOD性能、批量资产与源许可仍未验收，整单不记Done，不作为Shipping/cook或发布通过。
