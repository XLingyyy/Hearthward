# 新版UI及项目根测试游戏GitHub同步 · 2026-10-04

Owner XLingyyy已明确授权将全部UI更新、根目录测试游戏和相关资料上传至 `XLingyyy/Hearthward`，并在PR中写明测试端位置。授权快照提交为 `4f4144cbec897d956fdd5ed0f3872b0840904bb8`，实施分支 `codex/TASK-053-title-wheel`，受测UI基线 `4db5789184fe38e041d62a1e68c8517338ea0b01`。远端main已新增TASK-052时间集成至 `67fb0784ca8c6d488173e587e7f95c4be0d9092a`；本次保留当前UI受测版本，组合集成及main合并另行评审。

## 测试游戏在仓库中的准确位置

GitHub仓库根目录对应本地 `E:\AiAgent\XLingGame\Hearthward`，不包含上层XLingGame目录。PR合并前选择 `codex/TASK-053-title-wheel` 分支。根目录已有直接可用的对应入口，不依赖上层本机转发脚本。

| 仓库路径 | 作用 |
| --- | --- |
| [启动测试版游戏.cmd](../../../../启动测试版游戏.cmd) | 正常最新版游戏入口，从登录主界面进入／继续 |
| [TestClient/启动测试端.cmd](../../../../TestClient/启动测试端.cmd) | 正常入口调用的统一测试启动器 |
| [TestClient/README.md](../../../../TestClient/README.md) | 环境、操作、存档隔离及回归说明 |
| [scripts/ui/launch_test_client.py](../../../../scripts/ui/launch_test_client.py) | 启动当前Development工程、复用人工测试档 |
| [地图测试版.cmd](../../../../地图测试版.cmd) | 独立新档自动打开新版M地图 |
| [地图测试版_预览.png](../../../../地图测试版_预览.png)、[地图测试版_移动验证.png](../../../../地图测试版_移动验证.png) | 原本位于根目录的地图预览与移动验证截图 |
| `Source/Hearthward/UI/`、`Resources/UI/` | UI实现、布局、字体和美术；二进制经Git LFS管理 |
| `docs/qa/TASK-053/`、`scripts/ui/` | 全部历次UI验证资料、截图和专项启动／回归工具 |

实际入口链为 `启动测试版游戏.cmd` → `TestClient/启动测试端.cmd` → `scripts/ui/launch_test_client.py` → `Hearthward.uproject`的UE Development独立游戏。此处“测试游戏”是源码测试端，依赖本机UE 5.8.2、MSVC／Windows SDK、Python 3.10+、GameFactory及Development编译结果。首次克隆取回Git LFS资源，配置 `HEARTHWARD_UE_ROOT`／`HEARTHWARD_FACTORY_ROOT`／`HEARTHWARD_PYTHON`，按根README准备本地AI，再运行 `python -X utf8 scripts/ui/verify_input_client.py --build --label first_build`，成功后双击正常入口。未将旧0.1.0发行安装包冒充本轮最新程序，最新地图／背包／日志之后没有重新Shipping打包。

## 上传范围

本次归档标题／登录、设置、存读档、HUD、背包、行装管理、技能、日志、暂停、营地建造、加载及地图的全部现有更新。包含黑金炭灰布局、标题滚轮菜单、确认框高亮修复、加载后输入恢复、退出返回登录、提示两秒淡出、四栏道具与药食3秒互斥、拖放装备／移位／快捷配置、穿戴去重、行装管理返回背包、收集要素连续列表，以及灰暗写实地形、暗烟遮雾、局部折角边缘、统一灰蓝水面和实时红蓝火焰。

相关Building／Campaign／Combat／Experience／Gameplay／Input／Save／Survival源码及原生测试、CT-004接口增量、版本号和安装脚本一并归档，避免只上传视觉代码造成缺少接口。保留完整历史截图、报告、源指纹和失败尝试；各轮PASS仍仅绑定各自记录的受测源码。现有美术和截图使用既有LFS规则，新增根移动验证图逐字节复制自上层根目录。`.cmd`新增显式CRLF检出规则，确保不同Git换行配置下的Windows启动入口一致。

`TestClient/Profile/`、`Runs/`、`Logs/`、`Backups/`、`client.json`、`Saved/`、`Binaries/`、模型权重与运行库仍保留本机，未上传玩家存档／设置、生成缓存或安装备份。生成的本地回归文件不会随TestClient源码进入仓库。

## 验证绑定

上传前核对最近成功构建的230项编译输入、正常根入口的282项运行输入，全部与当前本地Source／DLL／UI资源一致，见 `runtime-fingerprint-check.json`；此次同步未修改功能源码。当前Development构建、日志／PIE124项、根入口输入144项和启动11项的原始证据位于 [journal-compact](../journal-compact/REPORT.md)。地图56／29／12项与行装307项等专项属于各自历史绑定版本，不据上传提交推定重新通过。

本次仓库检查0错误、基于授权快照的路径检查0错误、33/33工具测试及暂存差异格式检查通过。暂存图片共2451个路径、去重992个LFS对象约1.135GB，全部LFS指针、大小及SHA256逐项核对通过；`delivery-manifest.json`列出快照文件与Git blob（不含清单自身及后加纯文档记录）。提交及远端核验另绑定实际实现SHA。首次资料检查发现本报告尚未创建导致四个相对链接缺失，创建报告后重跑；初次结果保留为 `preflight-*.txt`。暂存差异检查发现原始CRLF快照与编译器日志被解释成文本尾随空白；将这些只读归档按原字节保存为二进制差异，修正一份报告末尾空行后通过，问题路径保留为 `first-staged-diff-summary.json`，功能源码未改。未绕过检查、伪造评审、合并main或更新GitHub正式Release。

回滚方式：关闭本轮测试窗口后检出上一受测分支／提交，并使用对应Development构建；人工测试资料继续由TestClient隔离保存。公共存档增量为可选字段，旧档回退行为和验证详见CT-004及四栏／拖放专项报告。与TASK-052组合后的保存、世界时间及安全旅行行为仍需集成验证。
