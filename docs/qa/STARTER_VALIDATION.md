# 启动包本地校验记录

日期：2026-09-17（项目文档口径）；对象：工作流启动包 v1.0。
环境：Linux，Python 3.13.5，Git 2.47.3。仓库辅助工具声明最低Python 3.10；本次未在3.10或Windows执行。
交付目录不是用户的GitHub仓库，没有实际项目提交SHA，因此下方使用交付文件SHA256定位本次对象。

## 实际结果

| 检查 | 命令或方式 | 实际结果 |
|---|---|---|
| 文档与任务结构 | `python scripts/validate_repo.py` | PASS；10份任务快照；0错误 |
| 工具单元与本地Git场景测试 | `python -m unittest discover -s scripts/tests -v` | PASS；29项执行，0失败，0跳过 |
| 已提交／暂存／未暂存／未跟踪范围 | 独立临时Git仓库测试 | 覆盖越界新增、删除、重命名、已提交越界；基线范围不能被当前分支自行扩大 |
| 只读接手 | 临时Git任务分支，保留既有未提交文件 | 原文件字节、HEAD、git status保持不变 |
| 非Git目录接手 | `python scripts/agent_context.py --task TASK-010` | 返回2，明确GIT_STATUS_UNAVAILABLE；不伪造分支和HEAD |
| 未配置启动门槛 | `python scripts/validate_repo.py --launch-ready` | 返回1，20项缺失；这是预期拦截，不表示M0通过 |
| YAML语法 | 本次辅助检查使用PyYAML BaseLoader解析三份YAML | PASS；只证明语法与所检查字段，未在GitHub运行 |
| 主文档目录 | 22个显式锚点与22个目录入口核对 | PASS |
| 设计归档 | 与原上传v0.3 DOCX逐字节比较 | 一致；提取文哈希与MANIFEST匹配 |
| 未定规则迁移 | R01—R25在登记表各出现一次 | PASS；没有将任何R项擅自改成已确认 |

工具测试在一次性临时Git仓库内创建本地提交，不访问远端，不安装LFS，不执行UE。
测试输出见 [29项测试日志](evidence/STARTER/repo-tools-unittest.txt)；
预期拦截输出见 [未配置M0检查](evidence/STARTER/launch-ready-unconfigured.txt)。

## 尚未执行的项目验证

真实GitHub的PR检查、CODEOWNERS匹配、分支保护：NOT_RUN。
Git LFS安装、远端对象下载、两个真实账号加锁与推送演练：NOT_RUN；本环境没有git-lfs。
LFS指针测试只使用构造的指针和OID，不代表远端服务验证通过。
UE打开、C++编译、蓝图加载、自动化测试、打包、双机复现、游戏试玩：NOT_RUN。
真实模型加载、RAG检索、弟弟实际动作与回档验证：NOT_RUN。
本记录不改变PROJECT_STATE中的游戏进度，也不能替任何游戏验收项目打勾。

## 本次检查的关键文件指纹

| 文件 | SHA256 |
|---|---|
| `WORKFLOW.md` | `c3ad952b4703cff874f14c73a7253a9b80276638c52a8b5aab49d0fece4890e6` |
| `scripts/agent_context.py` | `d116faa6c575578a67911f8cf3c907fdbfd2be0f8d23d866693fbc72dd24ebfb` |
| `scripts/validate_repo.py` | `0b882c6a193b5481ad5b32722e8a984b3bfcafb4f14ede93910a20b2a94d2edc` |
| `scripts/tests/test_repo_tools.py` | `dab2a3db3536dd4c967f2b8457b50f81ca3c46eedb187596b4c290073b492376` |
| `.github/workflows/repo-checks.yml` | `f431da873a86e1d57457ace14f3d3e71856bd66551439b34b4f52b2b226a3015` |

任一上述文件发生变化，本记录只保留为历史证据；采用仓库后以真实tested_commit建立新报告。
本地校验是工程辅助，不是权限沙箱、源控锁服务或完整安全审计。
