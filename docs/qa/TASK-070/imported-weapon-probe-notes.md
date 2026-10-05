# TASK-070 两件武器导入后读取／修复草稿

2026-10-04。仅在本代理 QA 目录准备脚本；未运行 UE、修改 Content、提交 Git 或做持握／风格验收。根代理按已登记的12包完成公开导入和锁定后运行。

运行证据更新：根 `docs/qa/TASK-070/spear-public-import.json` 确认长矛全部六个精确包通过公开import生成，warnings／errors空，StaticMesh＋四Texture2D＋同名MaterialInstanceConstant。先前UMaterial类型预测已由真实导入结果纠正；无需补图。根报告生命周期closedtrue；本代理没有执行该导入。短刀仍按其实际导入报告独立检查。

## 只读 probe

`probe_imported_weapons.py` 读取相邻 `first-weapon-import-manifest.json` 的两个 destination，逐件加载 StaticMesh、MaterialInterface 和 Color／Normal／Roughness／Metallic。默认只写当前 UE 项目的 `Saved/Task070/equipment-import-probe.json`。可指定输出文件区分导入后、补图后、修复后三个状态。

根在已有 UE Python 执行入口内运行以下代码；本稿没有调用私有 UEClient transport 或另建导入器：

```python
import runpy
qa = r"G:/GameFactory/Hearthward/.agent-local/task055-combat/docs/qa/TASK-070"
draft = runpy.run_path(qa + "/probe_imported_weapons.py", run_name="task070_probe_draft")
report = draft["probe"](
    output_path=unreal.Paths.project_saved_dir() + "Task070/equipment-import-probe-after-import.json"
)
```

报告记录真实 asset class、UE local cm Bounds、每LOD顶点／三角面／UV／render sections和其材质索引、slot实际材料、simple／convex collision数量、Grip／GripSocket／BladeBase／BladeTip／Tip存在及变换、纹理尺寸／sRGB／compression／flip-green／virtual-texture状态。ImportData保留实际文件路径，并分别读 legacy FBX 导入偏移／场景转换和 Interchange translator／pipeline偏移／BakeMeshes设置；不按原始FBX坐标猜导入变换。

材质记录所有已保存表达式的实际输入节点、输入名／连接输出、纹理／sampler，以及公开 MaterialProperty 可访问的 surface 输入当前连接。函数调用保留真实函数路径，明确不展开函数内部；隐藏的CustomData／CustomizedUV等输入不使用数字枚举绕过公开API。`missing`、实际类型不符和`read_failed`独立记载，不能视作通过。probe没有任何资产 set／connect／save／collision／socket mutation；Python AST已检查。尚未实际运行 UE，API调用结果仍待根采集。

MIC适配增加：读取实际parent链、显式texture／scalar／vector overrides（保留parameter_info的association／index）、公开getters给出的继承后GlobalParameter有效texture／scalar／vector值及静态开关。最终UMaterial parent graph仅只读，表达式记录实际parameter_name，将实例参数与parent输入关系连起来。name-based API不提供全部layer index，因此layer的继承后值不冒称完整测量；函数内部仍明确未展开。没有Modify／save parent。

## 缺图走公开资产入口

对报告确证缺失的任一纹理，根在现有public UEClient环境中复制该件的身份 descriptor，仅修改 `artifact_key`，使用既有meta中真实文件：

```python
source = dict(entry["source"])
source["artifact_key"] = "roughness_texture_path"  # 或metallic/color/normal_texture_path
result = ue.assets.import_texture(source, destination=entry["destination"])
```

本机公开 `engine_adapters/ue5/assets/client.py:280` 已确认此签名。无需 AssetTools 旁路，也无需新生成／付费调用。读取actual result后重新probe，不能仅依据源文件存在宣称UE纹理导入成功。

## 明确PBR缺陷的独立修复

`fix_confirmed_weapon_material.py` 加载时不施工，根审阅实际报告并授权具体错误通道后才显式调用。只能处理现有UMaterial；MaterialAttributes或VirtualTexture路径会停下要求按实际图审阅。本稿不新建material、不修改其他材质输入、不改Mesh／Socket／组件。

```python
fix = runpy.run_path(qa + "/fix_confirmed_weapon_material.py", run_name="task070_material_fix_draft")
result = fix["apply_material_fix"]("spear", ["Roughness", "Metallic"])
```

以上通道只是调用示例，实际通道以probe证据和根授权为准。四张图必须全部真实加载；缺图先公开补导。匹配纹理的已有TextureSample优先复用，只有所需图没有采样节点才增加节点。BaseColor／Normal连接第一输出（本机TextureSample首输出实际为RGB）；Roughness／Metallic连接R。Normal明确TC_NORMALMAP／sRGB false；数据图sRGB false，既有Default或Masks压缩保留，错误的其他压缩才改Masks并匹配sampler。BaseColor保留正常现有压缩，避免无依据重压缩。flip-green／材质单双面／blend／其他输入不改。

长矛实际为MIC，上述UMaterial graph修复函数会拒绝它。仅当probe确证MIC某个texture override错误，根使用独立 `apply_instance_texture_fix(piece, confirmed_bindings)`：bindings的key必须是probe读到的真实parent既有GlobalParameter名称，value只可为已导入的Color／Normal／Roughness／Metallic。参数名不按通道名猜测。函数预检所有请求的parent参数／纹理，再公开set_material_instance_texture_parameter_value；setter内部已UpdateMaterialInstance，无需额外recompile共享parent。只保存该MIC，不创建材料、不改parent、不改纹理设置／scalar／switch。

本机5.8 `MaterialEditingLibrary.cpp:1507–1515` setter执行实例修改后仍返回未置true的bResult。本稿记录返回bool，并以公开getter返回真实texture及is_material_instance_parameter_overridden确证后态；不因这个已查明的返回值缺陷修改Engine或误判施工失败。API调用仍待根实际验证。若根尚未copy旧probe，可直接copy更新稿；`mic-probe-adaptation.patch`相对前次已交付的脚本版本，便于局部审阅。

完成后公开 `recompile_material` 获取当次compiler errors，成功才显式save材料和改过的纹理，再用只读probe记录后态。返回空compiler errors仍需根真实渲染核对，不冒称材质视觉PASS。API中途失败可能留下编辑器dirty对象，没有承诺事务回滚；根应查清失败后再保存。没有删除旧表达式、重写整个graph或隐藏错误。

## 自动碰撞和NoCollision

实际probe确认simple／convex大于零后，根在已锁的该mesh包上使用公开StaticMeshEditorSubsystem。此操作独立于PBR修复：

```python
mesh = unreal.load_asset(entry["expected_mesh_package"])
sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
before = (sm.get_simple_collision_count(mesh), sm.get_convex_collision_count(mesh))
if before[0] > 0 or before[1] > 0:
    removed = sm.remove_collisions(mesh)
    after = (sm.get_simple_collision_count(mesh), sm.get_convex_collision_count(mesh))
    if not removed or after != (0, 0):
        raise RuntimeError("Collision removal failed: " + str((before, after)))
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError("Mesh save failed")
```

`remove_collisions`本机Subsystem公开封装会应用mesh changes。simple count与convex count是资产数据；零计数不能证明实例组件已配置NoCollision。后续真实持握组件必须配置 `CollisionEnabled.NO_COLLISION`，权威伤害继续现有CombatSweep，不能将本轮导入报告计作055真实扫掠完成。

## 握点暂不施工

此前 `weapon-source-axis-facts.json` 是原始FBX控制点候选。上述probe先给真实Bounds和ImportData；根比对真实导入轴／尺度，并在StaticMesh Editor实物选点后再写Grip／BladeBase／BladeTip。原始PCA节点缩放后的长度估计仍不等同最终UE武器尺寸。此次不生成Socket mutation脚本、不生产绑定，不记手握贴合、动作窗口、Owner风格或授权ledger PASS。

## API证据

- 本机UE5.8 `StaticMesh.h`：get_bounds／get_num_lods／get_num_sections／get_num_triangles／get_num_vertices、find_socket。
- `StaticMeshEditorSubsystem.h:259/276/338/466`：simple／convex count、remove_collisions、LOD section材质索引；实现公开封装。
- `AssetImportData.h:149` 的ScriptName ExtractFilenames；InterchangeAssetImportData.h:151/167 的GetPipelines／GetTranslatorSettings。
- `MaterialEditingLibrary.cpp:1148/1262` 直接读取已保存表达式输入；无需先打开材质编辑器。`Material.cpp:6571` 的有效输入范围用于排除MP_MAX／deprecated／derived无效指针。
- `MaterialExpressions.cpp:2570` 的TextureSample首输出RGB；`MaterialEditingLibrary.cpp:81` 空输出名选择首输出。`RecompileMaterial:1035` 返回当前resource compiler errors。
- [Epic MaterialEditingLibrary公开API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary?application_version=5.6)、[MaterialSamplerType](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialSamplerType?application_version=5.6)、[MaterialProperty](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialProperty?application_version=5.6)。网页版本5.6供公开接口参考，本机5.8源码优先，尚无脚本runtime验证。
