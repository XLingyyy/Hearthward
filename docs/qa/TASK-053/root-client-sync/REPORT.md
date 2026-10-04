# 最新UI与地图接入根目录测试游戏 · 2026-10-04

根目录[启动测试版游戏.cmd](../../../../启动测试版游戏.cmd)已经接入当前常规TestClient游戏；上层XLingGame也有同名入口。正常进入登录页，保留原测试存档与设置，进入／继续后按M显示最新版地图。所有当前UI、统一湖岸灰蓝水色、折角轮廓、松散黑雾及真实红蓝人物火焰一起使用当前Development程序。

## 接入方式

两份根CMD转发到原TestClient入口，使用UTF-8／CRLF、已配置Python路径和参数转发。常规启动脚本采用与地图预览一致的NewConsole异步日志窗口，避免旧Windows控制台选择状态暂停游戏。默认入口保留原Profile和save_pool；隔离回归用新的GUID及TestClient/Runs，并在原UEClient内停止自有验证进程。

正常游戏直接读取当前项目Resources/UI，并使用当前DLL，无旧地图副本或DLL替换。最新地图原241项源码／资源指纹全部仍匹配；其中237项实际运行输入共同包含于本轮282项启动指纹。原[地图专项验证](../local-map-water/REPORT.md)的56项游戏、29项图像与12项水色检查仍绑定这些相同文件，本轮没有重跑或改写其旧结果。

## 当前证据

- [最终常规游戏回归](verify-normal/report.json)：144/144，从普通登录页进入，覆盖新游戏／继续／读档、实际GameViewport的M与其他界面快捷键、暂停策略及返回登录。无地图自动预览启动标记。
- [回归启动清单](verify-normal/source-manifest.json)与[默认根入口清单](root-source.json)：282项文件均匹配当前内容，最新运行输入237项与地图专项相同；原编译源码及DLL保持，复用前轮成功的[Development构建](../local-map-angular/build_3fb31c6c/build.json)，本次未编译。
- [真实根CMD启动](root-launch-smoke.json)：10/10，PID39556，正常Bootstrap加载结束，使用原人工Profile及save_pool，三次Development窗口响应。观察后保留游戏窗口供用户继续。
- [资料保留](preserved-data.json)：16份人工存档／设置及其备份逐字节保持。备份位于被Git忽略的TestClient/Backups/20261004-root-client-sync；正常游戏可能新增运行诊断，单独列入清单。
- 工具测试33/33、仓库0错误、差异格式及[当前增量范围](incremental-scope.json)通过。正式基线范围检查依旧缺少TASK-053获批快照而FAIL，该既有限制单独记录，未用增量核验代替。

回归实际登录页：

![普通游戏登录页](verify-normal/boot-title.png)

## 复现

双击根目录正常游戏入口，选择继续或新游戏后按M查看。原TestClient/启动测试端.cmd和地图测试版.cmd专项预览继续可用。

自动回归从上层根目录启动，选择新的绝对输出路径：

```text
启动测试版游戏.cmd --verify-input E:\AiAgent\XLingGame\Hearthward\docs\qa\TASK-053\root-client-sync\new-run\report.json
```

脚本拒绝已有报告以免误用旧结果。首轮144项通过后补齐上层便利入口的可选文件检查及已有报告保护，再次完成最终144项，前轮证据保存于verify-normal-before-portability-fix。

只更新项目内Development测试游戏，没有改三维模型、玩法、存档格式、C盘安装或旧发行包。没有提交、推送、合并、Shipping打包或公开发布；Owner实际键鼠游玩与视觉验收仍待进行。
