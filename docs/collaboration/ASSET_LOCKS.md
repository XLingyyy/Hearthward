# 资产锁操作规则

完整规则见 [WORKFLOW第7节](../../WORKFLOW.md#s07)。本文件不是实时锁表。

实时锁由Git LFS服务维护，责任声明放当前任务Issue。不要多人写一个Git内LOCKS文件冒充互斥。
共享代码文件没有二进制锁也仍须预约Owner窗口。一个人启动多个Agent时，它们可能使用同一凭据，
LFS无法据此自动区分会话，所以“一任务一写者”仍必需。

## 首次演练

A确认路径存在并锁定→B在另一工作区用另一账号验证受阻→A提交PR、合并和验证→A解锁→B同步接手。
记录服务URL（不含token）、Git/LFS版本、文件、任务、锁ID、两名操作者与实际结果。
不要将服务错误解释成没有锁；如果回显锁验证建议，按当前远端URL启用并实际演练，
不在共享脚本写死个人或不明远端地址。

## 命令（真人授权且状态核对后）

```bash
git lfs version
git lfs env
git lfs locks
git check-attr filter lockable -- Content/Tribe/World/Example.umap
# 已获分配的真实路径替换示例；文件必须存在
git lfs lock "Content/Tribe/World/Example.umap"
# 编辑、测试、提交、PR合并并交接之后
git lfs unlock "Content/Tribe/World/Example.umap"
```

`git lfs env`只在本机核对；分享前检查是否包含个人端点信息。密钥不入日志。
服务端是否支持、客户端是否验证、只读属性是否正确必须实测，本包没有替你完成该演练。
强制解锁、历史迁移、丢弃资产只允许Owner明确审批，且先保留可恢复副本。
