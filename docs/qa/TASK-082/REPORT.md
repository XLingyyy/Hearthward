# TASK-078 地图更新与本地可玩包

当前火焰尖端更新、Source／DLL指纹和本地包以 [三角火焰尖端报告](FLAME-TIPS-REPORT.md) 为准；本文件保留此前版本证据。

本文件保留此前地图布局版本的历史指纹与结果；当前用户四项bug修复、源码验证与可玩包以 [地图与传送修复报告](TRAVEL-FIX-REPORT.md) 为准。

本文件是首次半径1000米圆形地图的历史报告。用户随后要求无黑雾可拖动矩形，并删除地图四周留边与外围文字；当前实现和交付以 [全屏地图报告](FULLSCREEN-REPORT.md) 为准。下列程序、DLL、Source指纹及PASS仅绑定此前圆形版本，未改写旧证据。

2026-10-06。用户要求扩大地图、补充小地点名称、使红蓝火焰指示各自朝向，并统一全部地图入口。实现位于本地分支 `codex/map-1000m-heading`，基线为已拉取的远端 main `ca21120cef4421d039c44248a0cfa5043c248feb`。当前成果尚未提交、推送或公开发布；本报告绑定 [Source／DLL／资源指纹](source-manifest.json) 和 [最终 Shipping 指纹](shipping-build-info.json)，不能只用基线提交代表改动后的程序。

## 实现结果

默认视图半径1000米、直径2000米，缩放1、平移0。旧配置实际为半径250米、直径500米；本次按用户明确要求的半径1000米实现。石堡附近使用原开场中心，在远处营地打开时以玩家位置为中心；任务与地点定位可移动同一视图。

底图为2400×2400、覆盖4032米正方形的地形图集，使用现有原始高度场、实际水域三角形、场景树石分布及137项建筑组件投影。开场原500米方形内63001个实际碰撞采样用于高度源对齐，P99误差0.0148米；这不是全4032米区域都做过碰撞采样的声明。制图参数及输入／输出指纹见 [cartography.json](cartography.json)。湖泊与河流统一灰蓝水色，保留原材料、灰暗山地层次和不规则烟雾边缘；不规则轮廓保留标准1000米圆面积的约90.98%。

石堡主堡、内庭、石堡侧门、故乡入口、后巷撤离口、湖东小径与故乡眺望处标注于对应位置；较拥挤的石堡地点使用引线。红色玩家与蓝色弟弟火焰分别读取各自实际 Actor 位置和旋转，围绕其定位尖点旋转，保留原颜色和闪烁。地图 +X 向右、+Y 向下；原火焰尖点朝下，因此旋转值为 Actor yaw−90°，yaw0指向右方、yaw90指向下方。

M、菜单地图、任务“地图定位”、已发现地点和传送共用同一地图资产、比例和投影。原世界地图版式与底图入口已移除，右上改为“地点与传送”面板，可筛选、分页、选择目的地和传送。探索仍为100米；未知任务位置、路标和传送资格沿用现有规则。地点面板展开后右键设置路标，逆坐标变换同步新地图比例。100%、125%、150%字体检查覆盖完整任务目标和按钮文字。

![默认地图](map-preview.png)

![地点与传送](map-locations.png)

朝向截图：[玩家 yaw0／东方](heading-east.png)、[玩家 yaw90](heading-south.png)。图中的蓝色弟弟分别使用−90和0度朝向，独立于玩家。

## 当前验证

环境：Windows、UE 5.8.2、VS 14.44、Windows SDK 10.0.26100.0。引擎操作通过显式指定本工程的 GameFactory `UEClient` 公开 API 执行。

| 检查 | 实际结果 | 证据 |
|---|---|---|
| Editor Development 最终构建 | PASS | [editor-build.json](editor-build.json) |
| `Hearthward.Map064.ExplorationBoundaries` 与 `Hearthward.UI069.MapScaledGuidanceKeepsCompleteText` | 2/2通过、0错误；每项1个隔离夹具警告 | [native-index.json](native-index.json) |
| `python -X utf8 scripts/ui/launch_map_test.py --verify` | 74/74通过 | [runtime-report.json](runtime-report.json) |
| `python -X utf8 scripts/ui/audit_expanded_map.py <当前运行目录>` | 17/17通过，Source／DLL／资源无指纹差异 | [expanded-audit.json](expanded-audit.json) |
| `python -m unittest discover -s scripts/tests -v` | 33/33通过 | [tools-tests.log](tools-tests.log) |
| Win64 Shipping Build/Cook/Stage/Archive | PASS | [package-result.json](package-result.json) |
| 最终 C++ Shipping 重新构建及归档程序替换 | PASS | [shipping-final-build.json](shipping-final-build.json) |
| 最终归档程序独立启动 | 6次采样均有窗口且正常响应 | [shipping-startup.json](shipping-startup.json) |
| ZIP全部152个文件CRC校验 | PASS | [zip-integrity.json](zip-integrity.json) |

原生测试警告为 `UEnhancedInputLocalPlayerSubsystem` 在隔离测试 LocalPlayer 中缺少有效 `PlayerInput`；两项测试本身均成功，未隐藏警告或声称零警告。原生回归覆盖探索与路径知晓规则、已发现地点、任务地图入口、同一投影、任务全文、缩放字体和传送按钮命中。

运行检查使用实际 Development Standalone 新游戏、真实 Widget／Slate 事件、Actor 与碰撞数据，采用独立GUID档池；覆盖 M 开关、五组双方朝向、真实坐标、缩放／平移、烟雾裁剪、缺失弟弟及暂停／不暂停状态。1280×720、1600×1000、1920×1080、2560×1080和缩放平移图像中，轮廓外探针像素差均为0。此证据不等于 Windows 物理键鼠输入或完整真人通关验收。

原始运行目录为 `.agent-local/qa/TASK-078/verify_e265c8e8`，原生目录为 `.agent-local/qa/TASK-078/native_b3d22bfa`。初次观察使用的旧卧室坐标断言与实际石堡新开场相差1米，失败记录保留于忽略目录；当前按实际开场与2米容差检查并通过，未用旧失败运行充当验收结果。

普通仓库检查、路径范围自检及差异检查记录见 [repository-checks.json](repository-checks.json)。新任务在基线提交中没有批准快照，因此 `validate_repo.py --task TASK-078 --base ca21120c…` 的基线批准前置条件不满足；未改动验证器或伪造历史批准。当前任务 allowed_paths 的直接比对另行记录。

## 本地交付

上层项目根目录为 `E:/AiAgent/XLingGame`。双击 `启动地图更新版.cmd` 或 `启动正式游玩版.cmd`，均启动 `Hearthward-Playable-20261006-Map1000/Windows/Hearthward.exe`。需要搬运时完整解压 `Hearthward-Playable-20261006-Map1000.zip`，运行其中的“启动游戏.cmd”；保留整个 Windows 目录。随包包含模型和 CPU／Vulkan 运行库，无需 UE 或 Python。

完整资源在 Build/Cook/Stage/Archive 阶段生成。之后的最终变化只涉及 C++，已重新链接单体 Shipping 程序并替换归档程序，烘焙资产没有变化；最终 Source／UI 指纹全部与当前工程一致，归档中的配置与底图也逐文件匹配当前资源。程序 SHA256 为 `f5b0fb6de761e4fbcbb9076cd9e2e28ab0ec898d559ac7de06b1960517ea628e`。ZIP为4911454901字节（约4.91GB）、152个文件，全部成员CRC通过；SHA256见 [delivery.json](delivery.json)。原先的 Latest 目录和ZIP保留。

这份交付为当前可玩开发预览。本次没有重跑新包完整剧情、长时间性能、所有自由语言请求或第二台机器验收；项目原有的精细美术与完整游戏验收待办继续适用。
