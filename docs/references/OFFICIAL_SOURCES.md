# 官方资料与适用边界

核验日期：2026-09-17。工具事实与本团队拟采用政策分开；页面默认版本不等于工程锁定版本。
本包没有替用户仓库进行线上部署或权限测试。

## S1｜GitHub — About protected branches

[官方出处](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches)

PR、检查、管理员绕过与功能适用范围；不代表本仓库已配置。

## S2｜GitHub — About code owners

[官方出处](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners)

CODEOWNERS需有效账号与权限，不能当锁；多个Owner不是默认全部审批。

## S3｜Git — git-worktree

[官方出处](https://git-scm.com/docs/git-worktree)

多工作目录；仍共享仓库元数据，不是权限沙箱。

## S4｜GitHub — About Git Large File Storage

[官方出处](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-git-large-file-storage)

Git内指针与外部对象；容量／带宽按实际账户核查。

## S5｜Git LFS — git-lfs-lock

[官方出处](https://raw.githubusercontent.com/git-lfs/git-lfs/main/docs/man/git-lfs-lock.adoc)

本地存在的路径、服务器锁及推送验证。

## S6｜Git LFS — git-lfs-config

[官方出处](https://raw.githubusercontent.com/git-lfs/git-lfs/main/docs/man/git-lfs-config.adoc)

locksverify、客户端检查和只读属性；不可宣称不可绕过。

## S7｜Git LFS — git-lfs-unlock

[官方出处](https://raw.githubusercontent.com/git-lfs/git-lfs/main/docs/man/git-lfs-unlock.adoc)

解锁与force的风险；本包不自动强制解锁。

## S8｜Epic Games — One File Per Actor

[官方出处](https://dev.epicgames.com/documentation/en-us/unreal-engine/one-file-per-actor-in-unreal-engine)

外部Actor降低重叠、相关包不能遗漏；某些变更集工具依赖Perforce。

## S9｜Epic Games — Multi-User Editing Overview

[官方出处](https://dev.epicgames.com/documentation/en-us/unreal-engine/multi-user-editing-overview-for-unreal-engine)

协作会话不替代版本控制，约定基线与唯一提交者。

## S10｜OpenAI — Custom instructions with AGENTS.md

[官方出处](https://developers.openai.com/codex/guides/agents-md)

AGENTS加载与路径／配置相关；不对全部Agent作统一自动加载保证。

## S11｜Anthropic — How Claude remembers your project

[官方出处](https://code.claude.com/docs/en/memory)

CLAUDE.md以及@AGENTS.md引用；说明是上下文而非强权限限制。

## S12｜GitHub — Repository custom instructions

[官方出处](https://docs.github.com/en/copilot/how-tos/copilot-on-github/customize-copilot/add-custom-instructions/add-repository-instructions)

Copilot仓库入口；不同使用界面仍需验证加载。

## S13｜GitHub — Secure use reference

[官方出处](https://docs.github.com/en/actions/reference/security/secure-use)

最小权限、固定依赖与不可信代码／自托管Runner风险。

## S14｜Epic Games — Run Automation Tests

[官方出处](https://dev.epicgames.com/documentation/en-us/unreal-engine/run-automation-tests-in-unreal-engine)

编辑器和命令行测试入口；本项目测试组未建立。

## S15｜actions/checkout — pinned action metadata

[官方出处](https://raw.githubusercontent.com/actions/checkout/11bd71901bbe5b1630ceea73d27597364c9af683/action.yml)

本包固定revision的action.yml已查阅；不是对未来版本安全的永久保证。
