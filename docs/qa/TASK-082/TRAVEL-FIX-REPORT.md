# TASK-078 地点、旗帜与传送修复

当前火焰尖端更新、Source／DLL指纹和本地包以 [三角火焰尖端报告](FLAME-TIPS-REPORT.md) 为准；本文件保留此前版本证据。

2026-10-06，接续用户四项bug要求。分支 `codex/map-1000m-heading`，已同步的远端main基线 `ca21120cef4421d039c44248a0cfa5043c248feb`。修复与当前可玩包保存在本地，未提交、推送或公开发布。任务仍为Active，没有代填人工验收。当前代码绑定 [地图Source／DLL指纹](travel-fix/source-manifest.json)、[传送Source／DLL指纹](travel-fix/travel-source-manifest.json)及[Shipping程序指纹](travel-fix/shipping-build-info.json)。

“地点与传送”使用剧情数据中的实际类型，过滤遗物包、其他持久任务物品、人物和救援对象；任务指引仍能定位这些对象。已发现占领旗帜在地图真实坐标显示金色旗帜，已占领时变为绿色。住区旗帜标注为“占领点”，点击可查看地点性质和占领方法；发现提示也说明这是占领地点，并提示按M查看。序章先撤离，返回故乡后清理该区敌军、靠近旗帜按E并站定5秒；移动或受袭会中断。旗帜不提供传送按钮。

传送拒绝说明当前真实限制与解决方法：序章根据已取得护符／下达指令的进度给出下一步；故乡说明永久夺回条件；未激活目的地提示路标名称与E激活；距离出发点过远说明最近已激活路标、实际距离及2米交互范围；正在占旗、采集、建造、加载、结算或倒地时给出相应原因。相同出发点也说明无需传送。剧情层反馈会传回界面，避免保留旧的“正在准备目的地，请稍候”。

地图的OwnPause会临时暂停世界，旧传送直接检查世界时钟Suspended，因此地图自己就能拦截有效请求。现在只释放该地图拥有的暂停：接受后关闭地图并开始加载；拒绝后恢复原暂停和选中地点。其他界面或动作造成的暂停／限制继续生效。

目的地区域加载完成后，本地导航网格的生成可能尚未开始或处于两批构建之间。原逻辑依赖全世界的导航构建状态，会等待无关区域，或过早将尚未生成的渡口落点判为不可通行。现在只验证目的地实际地面、导航投影、角色碰撞和各参与者的独立落点，并在45秒期限内重试目的地导航；不绕过导航或碰撞检查。验证全部参与者后才提交位置和参与战斗敌人的恢复，失败保留原位置及状态。

3000米×2000米无黑雾矩形、全屏地形、左键拖动、鼠标位置缩放、真实建筑、水域、地名和红蓝朝向火焰保留。右上入口仍可展开、收起；没有恢复外围深色留边或文字说明。未改主地图资产、存档格式、模型或剧情解锁规则。

![住区旗帜地点与占领指引](travel-fix/flag-guide.png)

[150%字体](travel-fix/flag-guide-150.png)、[序章拒绝原因](travel-fix/travel-prologue-reason.png)、[目的地激活指引](travel-fix/travel-activation-reason.png)、[已在当前路标](travel-fix/travel-same-station.png)。

## 当前验证

环境：Windows、UE5.8.2、VS14.44、Windows SDK10.0.26100.0。所有引擎操作通过显式指定本工程的GameFactory `UEClient` 公开API执行。图像审计使用本机含numpy／Pillow的Python。

| 检查 | 实际结果 | 证据 |
|---|---|---|
| Editor Development最终构建 | PASS | [editor-build.json](travel-fix/editor-build.json) |
| 地图探索、矩形拖动、字体及地点／拒绝反馈原生回归 | 4/4通过，0错误，各1项夹具警告 | [native-index.json](travel-fix/native-index.json) |
| 当前真实地图Widget运行检查 | 98/98通过 | [runtime-report.json](travel-fix/runtime-report.json) |
| 新游戏剧情交互、路标、往返及失败事务检查 | 37/37通过 | [travel-report.json](travel-fix/travel-report.json) |
| 图像／地理／当前指纹审计 | 44/44通过 | [rectangular-audit.json](travel-fix/rectangular-audit.json) |
| 仓库工具测试 | 33/33通过 | [tools-tests.log](travel-fix/tools-tests.log) |
| Win64 Shipping Build/Cook/Stage/Archive | PASS | [package-result.json](travel-fix/package-result.json) |
| 独立Shipping启动 | 6次窗口响应采样通过 | [shipping-startup.json](travel-fix/shipping-startup.json) |
| ZIP全部152个成员CRC检查 | PASS | [zip-integrity.json](travel-fix/zip-integrity.json) |
| 普通仓库、差异、当前路径范围及最终指纹绑定 | PASS；基线批准快照检查前置条件不满足 | [repository-checks.json](travel-fix/repository-checks.json) |

原生回归最初先复现任务物品、旗帜与拒绝反馈问题：新增测试出现12条断言失败，其他旧测试通过，见 [修复前原生结果](travel-fix/native-red-index.json)及[对应源码／DLL指纹](travel-fix/native-red-source.json)。加入标记后，字体回归曾错误绑定同名动作的空白地图标记；已为地点列表行赋独立ID，字体检查继续测量真实按钮文字与尺寸，没有删除断言。最终四项仅保留隔离LocalPlayer缺少有效PlayerInput的初始化警告。

首次真实传送检查发现营地到渡口会在区域刚加载时过早失败，5项后续断言失败；保留 [中间失败状态](travel-fix/travel-intermediate-failure.json)及[对应指纹](travel-fix/travel-intermediate-source.json)。增加目的地导航重试后，最终37项全部通过；没有用失败运行代替验收结果。

最终新游戏夹具实际取回护符、叫弟弟跟随并从后巷撤离到营地，随后通过公开剧情交互激活渡口路标。两次传送都从世界已被地图暂停的状态点击“传送”，自动关闭地图并载入目的地。渡口到营地抵达XY=(-980,-750)米，营地到渡口抵达XY=(523.64,-1120)米；后者距离原路标约1.36米。两段加载期间世界有效时间与日历时间都保持冻结，未参与战斗的独立弟弟留在原地。随后把营地落点故意放到不存在地面的坐标，生产传送流程明确取消，玩家仍在渡口原落点。全部真实反馈、位置、暂停及加载状态见传送报告。

夹具采用独立GUID档池，播种已发现地点、预加载渡口并重定位演员以执行交互；这是Development实际新游戏、Widget、流送、地面碰撞和导航验证，不能等同Windows物理鼠标操作、玩家存档验收或完整真人通关。Shipping仅独立验证启动和窗口响应，未声称完整通关。

当前地图重新采集矩形内651个分布式地面碰撞点，全部命中，原高度源P99误差0.0266米；137项实际建筑组件位置和包围盒与图集输入一致。普通、超宽屏、放大平移和最小倍率六组图像的视口与屏幕边缘均由真实地形覆盖。见[当前地理记录](travel-fix/current-geography.json)、[制图输入摘录](travel-fix/atlas-geography.json)及[当前全屏预览](travel-fix/map-preview.png)。底图SHA256仍为 `62e3e14ede4abeb731edd50ba3a883752d52efe341ee64447ad71743e949832f`。

Editor DLL SHA256：`ae5ee19b07c20da8c6bd439468cc2eb3d0ee541235c2585fe742e862bd802ad5`。Shipping程序SHA256：`305c9381e5ca6be082cbc5cc45df10f96cb22811005f7e5202bb4c39511383ae`。当前Source、资源、DLL及程序指纹逐项绑定上述运行与交付，详见两个运行清单、Shipping BUILD-INFO及最终仓库检查。

复验命令：`python -X utf8 scripts/ui/launch_map_test.py --build`、`--travel`、`--verify`；图像命令 `scripts/ui/audit_rectangular_map.py <运行目录> --atlas-scene <制图输入场景>`；原生过滤器 `Hearthward.Map064+Hearthward.UI069.MapScaledGuidance+Hearthward.Map078`。本次原始目录：Editor `rect_build_c5db0ed4`、原生 `rect_native_c34493ac`、地图 `rect_verify_2e723c79`、传送 `rect_travel_47b0ac73`，均位于 `.agent-local/qa/TASK-078/`。

## 本地交付

最新版为 `E:/AiAgent/XLingGame/Hearthward-Playable-20261006-TravelFix/Windows`。上层项目根目录“启动正式游玩版.cmd”“启动地图更新版.cmd”“启动矩形地图版.cmd”都指向本包。模型与CPU／Vulkan运行库随包，无需UE或Python。完整解压 `Hearthward-Playable-20261006-TravelFix.zip` 后运行其中“启动游戏.cmd”；旧FullMap、RectMap、Map1000与Latest包保留。

ZIP大小4911460579字节，152个成员，全部CRC通过。SHA256：`0a22081fde3c0aba429d8561ab3d386d840706eff94b0150474f603d85715e5d`，见[交付记录](travel-fix/delivery.json)。这是当前可玩开发预览；精细美术、完整真人通关、长期性能与第二机器验收仍沿用原项目待办。

TASK-078在基线中没有批准快照，因此基线任务范围检查前置条件不满足；保留真实失败结果，未改验证器或制造批准。当前allowed_paths直接比对、普通仓库与最终Source／程序／资源绑定另行记录。本轮没有提交、推送或公开发布。
