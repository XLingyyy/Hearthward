# TASK-094 精确来源读取入口

2026-10-07。根Agent已实际执行：150包READ_COMPLETE_WITH_UNKNOWN_PROVENANCE，无API异常，自有Editor已停止；[原始结果](registry-sources-20261007-01/asset-sources.json)、[汇总](registry-sources-20261007-01/summary.json)与[派生表](../../assets/TASK-094/ACTUAL_SOURCE_PAIRING.csv)保留全部分母。脚本和manifest静态记录保留原准备阶段，不回填为真实验收。只读取证不编辑台账许可、Source/Content/地图/原制作源；不下载、不生成、不计算hash。

入口：[inspect_asset_sources.py](inspect_asset_sources.py)，输入：[ASSET_INSPECTION_TARGETS.json](ASSET_INSPECTION_TARGETS.json)，静态结果：[asset-inspection-static.json](asset-inspection-static.json)。输入保留原845行登记中的160条关注项：36 UNKNOWN为26个实际包与10个UNRESOLVED需求，自然源家族124个实际包，共150个精确包。两入口map已经包含于26个未知包内，不再额外加分母。10个未定需求不编造包路径，不借其他任务的新增SFX或模型来自动消除本台账UNKNOWN。

本轮Root使用复合检查入口运行该inspector与动画读取，真实参数见 [launch.json](registry-sources-20261007-01/launch.json)，退出见 [stop.json](registry-sources-20261007-01/stop.json)。下面为本单可独立复跑方式，已有run不得覆写：

```python
from engine_adapters.ue5 import UEClient
ue = UEClient(project_path='G:/GameFactory/Hearthward/Hearthward.uproject',
              ue_root='G:/UnrealEngine/UE_5.8')
launch = ue.runtime.launch_editor(extra_args=[
    '-NullRHI', '-NoSound', '-NoSplash', '-Unattended',
    '-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/TASK-094/inspect_asset_sources.py',
    '-Task094Run=registry-sources-20261007-01',
])
# Root等待结果后，在finally调用ue.runtime.stop_editor(launch['payload']['process_id'])，
# 并核实自有Editor退出；本文件没有启动或停止任何UE。
```

本轮实际输出 `G:/GameFactory/Hearthward/.agent-local/qa/TASK-094/registry-sources-20261007-01/asset-sources.json`，已原样归档。允许Root预先创建该run的日志/profile目录；若结果文件已存在会拒绝覆写。stdout/UE log仅记录安全status/output/attempted摘要。Editor自身默认startup-map策略由Root launcher负责；脚本不会get_asset/load任何World，不进入PIE，不打开本清单的入口地图。

## 主源与读取范围

- 本机 [IAssetRegistry.h](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/AssetRegistry/Public/AssetRegistry/IAssetRegistry.h:298)公开GetAssetsByPackageName，使用on-disk精确包；不get_assets_by_path、scan_paths、search_all_assets或枚举Content目录。WaitForCompletion只等待Editor已有发现流程，若仍loading/没有目标AssetData则NOT_READ，不记0引用。
- 同文件 [K2_GetDependencies](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/AssetRegistry/Public/AssetRegistry/IAssetRegistry.h:557)明确为on-disk references ONLY；get_dependencies/get_referencers仅150个精确包，hard/soft package及game/editor-only范围明确，management/searchable关闭。实际返回None/异常时count/packages=null；只有成功空数组才count0。
- [AssetImportData.h](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Engine/Classes/EditorFramework/AssetImportData.h:149)及[实现](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Engine/Private/EditorFramework/AssetImportData.cpp:278)确认extract_filenames返回官方resolved文件名。仅已知mesh/texture/animation导入类加载精确target资产并读取asset_import_data；World/Material等类不调用get_asset。实际异常记录NOT_READ/count=null，空导入记录不当作来源配对完成。
- [StaticMesh SourceFile tag](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Engine/Private/StaticMesh.cpp:6230)和[Texture tag](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Engine/Private/Texture.cpp:1482)来自ImportData SourceData JSON；公开AssetData.get_tag_value('SourceFile')保留真实RelativeFilename。tag JSON错误保持NOT_READ，不猜修或用basename匹配授权。
- [Epic AssetRegistryHelpers文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AssetRegistryHelpers)提供现有Registry入口与tag读取；Python依赖返回数组/None契约也按本机UFUNCTION和既有官方文档核对，本轮已实际验证当前5.8绑定读取150个精确包；目录扫描和完整运行使用不在该验证范围。

精确导入来源只对本项目art_source内的实际路径做stat；保留项目相对源名、文件PRESENT/MISSING/NOT_READ和大小，不读内容或计算摘要。外部用户路径不resolve/stat，只有basename与NOT_READ；Runtime/Fonts/Saved同样排除。URL只保留origin/path，移除用户信息、query、fragment与已识别key/token路径值，不网络访问，不打印原始URL或任意headers/命令。

## 结论边界

150个包的依赖与referencers是真实AssetRegistry保存记录；2个入口map只记录直接hard/soft依赖和本目标集合的交集。没有加载World或递归完整依赖闭包，没有证明WorldPartition外部Actor、C++动态spawn、实际角色/地形可见性。Material可列出目标内依赖的导入来源作为旁证；依赖读取失败或目标来源未读必须保留null，不能写成无源。依赖纹理来源不能推出几何作者。

读取完成最多为 READ_COMPLETE_WITH_UNKNOWN_PROVENANCE；任何目标API异常为PARTIAL_NOT_READ/NOT_READ。该状态不是T094-C01/C02完整验收、Owner风格签收或发行许可。每行ledger_licence_state与release_blocker原样保留，路径存在、ImportData配对、CC0家族文件夹均不自动批准许可。未知继续UNKNOWN，10个需求继续UNRESOLVED。

静态7项通过仅核对精确150+10选择、失败null语义、坏tag失败、URL去凭据、外部路径边界、已列art_source文件metadata以及无地图加载/包写/扫描入口。它没有调用unreal/UEClient或提供当前UE读取信用。Root已保留全部逐包结果，97个直接项目源和25个生成类依赖旁证可继续逐件核实；不将未读项默认为零依赖或配对完成。
