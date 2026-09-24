# Windows Demo 发布范围

Owner：XLingyyy。2026-09-24 用户明确要求构建当前最新完整可玩 Demo 并发布 GitHub Release。

基线 origin/main@68e68d8，工作分支 codex/demo-release-20260924。保留本地工程关联与 IDE 文件，不合并 main。允许为完成发行修改打包配置、必要的 Shipping 兼容代码、scripts/release、README 与本目录证据；不扩充玩法或重做资产。补齐上层 UEClient 的 package / launch_packaged 公共能力，用于 UAT 构建及独立包验证，保留适配器补丁以复现。

验收：Windows x64 Shipping 包含自然地图、双角色、交互 UI、玩法数据和本地 Qwen/llama.cpp 及许可；独立包启动和主要操作实测；分卷安装包、操作说明发布到 GitHub prerelease，明确尚未完成的正式游戏内容与硬件限制。

发行烘焙已确认 TASK-028 的15个材质存在 Color/Masks 采样器不匹配，纳入本次必要发行修复；仅修改报错材质节点并持有对应LFS锁，资产来源与玩法结构不变。
