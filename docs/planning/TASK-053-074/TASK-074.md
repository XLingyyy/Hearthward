# TASK-074｜准备首版发布候选和正式交付

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿076，阶段D，P0；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-073。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

已有Demo发行、安装器、模型部署和更新提示，当前main合并不等于新Release。本单从073验收来源准备一个可复查RC、版本／更新／存档兼容说明与正式交付材料；先完成本地候选，公开发布由Owner明确下令。

## RC内容与封装

冻结唯一源码SHA、批准运行表、资产包／Cook记录、UE5.8.2、Windows工具链、模型／llama.cpp锁和版本号。复用UEClient公开API及现有package_demo.py／安装器流程，正常入口L_Bootstrap，剔除调试入口、独立动物演示及测试素材的默认发行入口，保留正式地图和必要内容依赖。

随包携带许可允许分发的runtime／GGUF、字体／素材许可证、必要运行库、游戏配置与完整模型可用性提示；不含API密钥、私密玩家档、开发日志、Python虚拟环境、引擎或制作源。Cook清单显式覆盖正式地图、软引用模型／动作／材质／音效，不凭Editor存在判断已打包。

## 安装、更新和交付说明

交付Windows安装包／可用便携包按既有平台路线，说明最低／推荐实测配置、空间、安装启动、键鼠操作、离线模型首次准备、存档目录和备份、升级／卸载存档保留、已知问题和联系方式。更新通知沿现有正式GitHub Release机制，只显示真实发布版本，测试RC不冒充公共正式版。

发布说明列首版8主线15支线、四区／双营地、伙伴本地模型边界及实际验收，不声称全知Agent或任意命令。若录音仍暂缓且Owner批准字幕版范围，明确字幕内容与缺录音项；若未作该决定，音频门槛仍未闭合，不能生成无条件“全部通过”。

## 最终验证与授权边界

本地RC在072已记录第二机器上做最窄发行差异复查：干净安装→模型ready→普通新游戏→保存退出继续，升级代表旧档并确认原件保留，卸载不损毁存档。若cook／包内容变动触及实测资产，再验证相关关卡／功能；仅改文字不用重跑全篇十小时。

RC清单绑定完整来源、包版本、测试和已知问题，Owner验收后才可按新指令提交推送／创建Release／上传包／公告。设计批准不授权发布，本轮不执行这些动作，也不创建Issue、自动合并或远端设置变更。验证产物完整性只在发行清单确需校验时使用checksum，日常规划不计算。

## 建议施工范围

- `docs/releases/first-release/`
- `scripts/release/（仅当前打包／安装器必要调整）`
- `docs/releases/first-release/manifest.json（RC冻结后生成）`
- `README.md`
- `docs/PROJECT_STATE.md`
- `发行manifest／许可证／版本说明按实际文件登记`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-003-save-load](../../contracts/CT-003-save-load.md)、[CT-002-companion-command](../../contracts/CT-002-companion-command.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
