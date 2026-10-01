# 本次动物制作工具快照

这是当前制作所用GameFactory代码的局部快照。完整GameFactory基线SHA记录在[归档清单](../../../../docs/qa/project-progress-20261002/source_archive.json)的`factory_base_commit`；原仓库为https://github.com/OpenDCAI/GameFactory-3A。许可见[LICENSE](LICENSE)。

- `operators/gen_motion/funcs/animal_motion/`：动物动作规划、骨架校准、制作、导出、回读、预览和验收代码。
- `engine_adapters/ue5/`：本机用于导入、构建、反射和运行的适配器文件快照。
- `local-adapter-changes.patch`：相对上述完整基线的5个已跟踪适配器文件差异。
- `engine_adapters/ue5/plugin/A3GameAssetEditor/`：原生资产／动画检查插件源码。
- R3专项脚本另见[修正脚本](../../animal_motion/corrections_20261001_r3/scripts)。

复现导入工具时，使用独立GameFactory检出，先对补丁执行`git apply --check <本目录/local-adapter-changes.patch>`，再应用通过检查的补丁并复制上述动物模块和插件源码到对应路径。插件按完整GameFactory的既有UE插件流程编译、准备；本快照保留源码，编译二进制不入库。已有的本机工具检出已具备这些变更。

制作模块及R3诊断脚本保留原作者路径；复跑时按任务元数据替换输入／输出根目录。游戏仓库的`scripts/animals/paths.py`支持`HEARTHWARD_FACTORY_ROOT`和`HEARTHWARD_UE_ROOT`，并自动映射当前归档的FBX、Blender和元数据路径。完整GameFactory的依赖与引擎环境需按其文档事先准备。
