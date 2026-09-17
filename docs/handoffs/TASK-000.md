# TASK-000 本地准备交接

## 后续授权与本次交付

2026-09-17 用户进一步授权读取设计、拆分第一个小任务、提交推送本仓库。
已用 Python OOXML 解析原始 GDD 的正文、表格和全部附录，未使用 LibreOffice；未修改归档或关闭任何 R 项。
新增 [TASK-003 任务单](../tasks/TASK-003.md)及 JSON：建立工程和第三人称灰盒行走基线，当前 Backlog、尚未实现。
同步更新根指令、上手入口、状态和任务索引，解除旧的设计阅读限制。
本次按 WORKFLOW 第20.1节初始化原空仓库 main；提交作者使用核实账号的 GitHub noreply 邮箱。
本次完整校验见 [启动提交验证](../qa/evidence/TASK-000/INITIAL_BASELINE.md)。
下文为上一阶段本地准备的历史记录；其中“未授权读设计／提交推送”已由本节更新，其他未执行项目继续有效。

日期：2026-09-17。用户授权工作流与开发空间准备、连接指定 GitHub 仓库；暂不阅读设计文档。

## 当前工作区

- 根目录：Hearthward 独立仓库；分支 `codex/TASK-000-workflow-bootstrap`。
- HEAD / tested_commit：无初始提交，检查针对本地工作区；不得虚构 SHA。
- origin：`https://github.com/XLingyyy/Hearthward.git`；fetch 成功，远端无分支。
- 原有输入：docs 下工作流 Markdown、启动 ZIP、GDD DOCX，均为用户文件并保留。
- 新增文件：启动包 64 个文件，以及本任务交接、验证记录和本机忽略配置；均未提交、未推送。
- 本次写入范围：工作流文档、模板、仓库配置、本地 Git 配置；设计资料只原样导入。
- GitHub Issue、真人角色与评审人未分配；未创建远端任务、PR 或修改保护。

## 已完成

- 安装启动包，保留原始输入；根 WORKFLOW.md 作为唯一维护入口。
- 建立独立 Git、origin、任务分支，防止游戏文件误入上层工具仓库。
- Git LFS 使用 --local 安装，保留启动包资产规则，启用远端 locksverify。
- 读取远端仓库及锁列表成功；连接器确认公开仓库与管理权限。
- 核验 UE 5.8.1、Visual Studio、MSVC、SDK、Git/LFS/Python 安装版本，填写工具链草案。
- 本机路径写入被忽略的 .agent-local/environment.json；不更改全局环境。
- 明确上层 activate.ps1 默认指向 GameFactoryUE，后续须显式绑定 Hearthward 工程。
- 修复启动包 Windows 自测的 LFS 指针夹具换行；两项定向复测通过，其余测试沿用首轮有效结果。

## 验证

结果与可复现命令见 [本次验证记录](../qa/evidence/TASK-000/LOCAL_SETUP.md)。
完整 validate_repo.py 会读取设计资料并做归档哈希，本次不执行该入口。
只调用已有检查器的非设计检查函数；设计内容、设计链接和归档完整性验证均 NOT_RUN。
工具单元测试的归档数据是临时生成夹具，不读取真实 GDD。

## 剩余事项

1. 首次提交前确认真人 Git 作者；需要 gh 时恢复其网页登录。连接器权限不等于 CLI 登录有效。
2. 获首次提交／推送授权后建立 main 基线；当前用户只要求连接仓库，文件保留本地。
3. 分配负责人和独立评审人、采用工作流，创建真实 Issue，运行 repo-policy 后配置保护并演练。
4. 游戏工程初始化仍独立待办：创建 Hearthward 工程，UE 5.8.1 两个 Target 使用 V7，EngineAssociation=5.8。
5. 执行真实 Editor 构建、启动、打包、双账号 LFS 与两机验收；现阶段全部 NOT_RUN。
6. 设计阅读与玩法开发等待用户后续指令；不将启动包草案视作已批准设计／契约。

## 恢复与隔离

新会话先读 AGENTS、START_HERE、PROJECT_STATE、TASK-000 及本交接。
无初始提交时 agent_context 会报告 HEAD 不可用；这是实际状态，不得用上层仓库 SHA 替代。
初始基线创建前不使用 origin/main 范围检查或创建依赖其提交的 worktree。
未编辑 UE 资产、未获取资产写锁；不存在本任务待释放的锁。
若撤销准备，只处理本次新增配置和启动包文件；保留原始三个输入文件，禁止清理上层仓库或全局设置。
