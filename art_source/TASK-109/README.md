# 营地坐姿动作来源

- 作者：Quaternius；Universal Animation Library Standard（2026-10-11 获取）。
- 官方来源：https://quaternius.com/packs/universalanimationlibrary.html
- 下载页：https://quaternius.itch.io/universal-animation-library
- 使用免费 Standard 包的 Unreal-Godot/UAL1_Standard.glb；原始许可保留于 License.txt（CC0）。没有使用收费 Pro/Source 包。
- 使用 Sitting_Enter、Sitting_Idle_Loop、Sitting_Talking_Loop、Sitting_Exit。
- `retarget_camp_life.py` 在既有 Brother.blend 上运行，通过框架 world_delta 重定向，保留既有绑定骨架、尺寸和导出约定。族人共享 Brother 骨架；不改变人口或工作收益。
- 生成目录通过 `pipeline.common.paths.task_output_dir` 分配，`AAAGF_OUTPUT_ROOT` 可指向空间充足的盘。导入及接触验收见 TASK-109 QA。
