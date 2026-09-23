# TASK-004 制作源资产同步交接（2026-09-23）

本交接记录房屋建筑部件及树木的局部进度。TASK-004 仍为 Backlog，整单 UE 导入、展示与验收未完成。

## 分支与授权

- 基线：`origin/main`，`e729349a6dc462e4945351cc2ad6a6cc678656ce`。
- 资产分支：`assets/building-parts-trees-20260923`；内容提交 `6456373a4d45e2fb5e30f5caf40aae0cb95ba918`，已通过 [PR #29](https://github.com/XLingyyy/Hearthward/pull/29) 合入 main（合并提交 `c30baced644f4fff032e6046db4abf0be445357c`）。README 的 PR 链接和本交接的 LFS 锁结果在合并后另行补充。
- 用户授权：将本地 `Resource/Tripo/房屋建筑部件` 和 `Resource/polyhaven/树木` 的更新内容提交、推送并创建 PR。未授权合并或 UE 接入。

## 本次增量

- `art_source/TASK-004/Tripo/房屋建筑部件`：8 张参考 PNG、8 个静态 FBX、8 张 PNG 预览、逐件索引与来源说明。Tripo `v3.1-20260211`，45 credits/件，合计 360；任务 ID 见索引。没有提交 API key、上传令牌、签名 URL、本地脚本或运行日志。
- `art_source/TASK-004/polyhaven/树木`：新增 Fir Tree 01 和 Pine Tree 01 的 2 个 Blender 源文件及 36 张 4K 贴图，共 38 个文件、2,266,006,664 字节。原有 Island Tree 02 和 Jacaranda Tree 的 22 个文件保持不变。Poly Haven 来源、CC0 许可与文件数量见 `polyhaven/SOURCE.md`。
- 同步更新 `README.md`、TASK-004 任务快照与说明。源资产仍为候选；未制作 UE 资产、碰撞、LOD、动画或正式建筑方案。

## 核验

- 新增树木 38 个文件和建筑资产 24 个二进制文件与本地来源逐件 SHA-256 对比，62/62 一致；原有树木 22/22 与本地来源一致。
- 建筑资产 8/8 FBX 文件头有效，8/8 预览为真实 PNG，索引逐件对应；树木 36 张 PNG 文件头有效。两个 `.blend` 文件为 Zstandard 压缩容器，已与来源逐字节核对，未在 Blender 中打开。
- `python scripts/validate_repo.py`：PASS，0 errors。
- `python -m unittest discover -s scripts/tests -v`：PASS，31 tests。
- `python scripts/validate_repo.py --task TASK-004 --base origin/main`：仅 `README.md` 报 `OUT_OF_SCOPE`。该检查按旧基线任务快照判定路径；旧快照未列 README，而根 `AGENTS.md` 要求每次任务收尾更新 README。本分支已把 `README.md` 纳入更新后的 TASK-004 `allowed_paths`。其余 68 条变更路径均通过范围检查。
- 新增 62 个二进制文件均由 Git LFS 跟踪；本分支未修改既有锁定二进制资产。
- 新增 8 个 FBX 与 2 个 `.blend` 均已取得 Git LFS 锁（10/10），锁保留到集成交接。
- UE Editor、PIE、打包与目标机性能：NOT_RUN。本 PR 仅同步制作源文件。

新模型的实际比例、枢轴、背面、碰撞、材质、拼接、树木优化与引擎加载仍需在 TASK-004 后续实施中检验。建筑部件标记 `TEMP_VISUAL`，不将外观解释为正式建筑风格或营地等级。
