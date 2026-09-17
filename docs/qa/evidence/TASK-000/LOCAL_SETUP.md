# TASK-000 本机准备验证

日期：2026-09-17。tested_commit：无初始提交，以下结果仅对应本地工作区。

| 检查 | 结果与边界 |
|---|---|
| 独立仓库根目录 | PASS：git rev-parse 返回 Hearthward 根目录 |
| origin 抓取 | PASS：git fetch origin 退出 0，远端空仓库 |
| GitHub 连接器 | PASS：仓库公开，当前连接账户 admin/push 权限为 true |
| gh CLI | FAIL：HTTP 401 Bad credentials；未修改凭据 |
| LFS 本机安装与锁列表 | PASS：git lfs install --local、git lfs locks；无现有锁 |
| UE 与编译工具安装 | PASS：UE 5.8.1 CL 56057345；cl.exe 19.44.35228.0；SDK 10.0.22621.0 |
| 工具测试 | 首轮 29 项中 28 PASS、1 FAIL；修正 Windows 夹具换行后，受影响的 2 项定向复测 PASS |
| 非设计结构检查 | PASS：10 个任务快照、33 份非设计 Markdown 的字段、依赖与相对链接检查 |
| 本机忽略与资产属性 | PASS：本机环境、原始 ZIP/DOCX 已忽略；uasset/umap 应用 LFS 与 lockable |
| 游戏构建／试玩／打包 | NOT_RUN：尚无游戏工程 |
| 双账号锁／两机验收／远端保护演练 | NOT_RUN |
| 完整文档、设计索引与真实归档校验 | NOT_RUN：用户要求暂不读取设计文档 |

## 工具测试命令

在 Hearthward 根目录，用本机 environment.json 中的 Python 执行：

```powershell
python -X utf8 -m unittest discover -s scripts/tests -v
python -X utf8 -m unittest scripts.tests.test_repo_tools.DocumentTests.test_archive_pointer_not_claimed_as_downloaded scripts.tests.test_repo_tools.DocumentTests.test_archive_wrong_pointer_rejected -v
```

首次失败已定位为测试的 write_text 在 Windows 自动写 CRLF，导致 LF 格式的 LFS 指针头匹配失败。
两个指针夹具改用 write_bytes 明确写入 UTF-8/LF；未改变检查器或真实设计文件。
其余 27 项不受该夹具改动影响，保留首轮结果，不重复执行。

## 非设计结构检查命令

以下使用已有检查函数，排除设计与契约正文，不计算真实归档哈希。
完整 validate_repo.py、--launch-ready 与基于 origin/main 的范围检查均未执行。

```python
from pathlib import Path
import sys
sys.path.insert(0, 'scripts')
import validate_repo as checks
root = Path.cwd()
checks.require_git_root(root)
errors = [f'Missing: {rel}' for rel in checks.REQUIRED if not (root / rel).is_file()]
tasks = {p.stem: checks.load_json(p) for p in (root / 'docs/tasks').glob('TASK-*.json')}
for task in tasks.values():
    errors.extend(checks.validate_task(task, root, set(tasks)))
errors.extend(checks.dependency_cycles(tasks))
for path in root.rglob('*.md'):
    rel = path.relative_to(root)
    if rel.parts[0] in {'.git', '.agent-local'} or rel.parts[:2] in {('docs', 'design'), ('docs', 'contracts')}:
        continue
    errors.extend(checks.check_markdown_links(root, path))
print(errors)
raise SystemExit(bool(errors))
```

该命令仅核验文件存在、任务字段／依赖和非设计文档相对链接；不代表全量 L0 或 M0 验收。
