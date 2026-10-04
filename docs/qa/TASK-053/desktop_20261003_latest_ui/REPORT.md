# TASK-053 · 当前最新UI同步桌面（2026-10-03）

本机桌面游戏已从0.2.0-preview.20261003.1更新为 **0.2.0-preview.20261003.2 / Win64 Shipping**。双击桌面“归火”或原GPU／CPU启动cmd即可使用。用户本次明确授权同步当前UI，解除最新四项增量此前仅测试限制；当前全部UI均包含在安装中。

## 同步内容

- 暂停：删除原插画、归火标题及右侧任务；背景实时世界毛玻璃，五项纵向炭灰黑金菜单。
- 营地建造：炭灰雾面背景，左侧名称列表、右侧原说明和材料，原开始放置流程。
- 加载：用户灰烬原图等比例铺满上方整块区域并裁切溢出，三行新文案逐字保留。
- HUD：三条生命／饱食／体力紧密独立、旧金属厚边与四周反光，弟弟信息上移；原红黄绿、标签、数值、五秒任务及十米指引保留。
- 此前登录、设置、存档、背包、技能和日志及最新高亮修复继续包含。

## 构建与当前验证

Development Editor构建通过、无诊断／警告，107.68秒，见[构建结果](../desktop_latest_build_20261003/build-result.json)。执行现有四项定向验证，共 **157/157检查、35张PNG**；所有源码／资源／DLL指纹收尾匹配，[验证绑定](development-validation.json)。

| 界面 | 运行 | 检查 | PNG |
| --- | --- | --- | --- |
| pause | hud_pause_96b7a61ff9b0 | 24/24 | 7 |
| building | hud_building_82ce8ccaa4e4 | 63/63 | 11 |
| loading | hud_loading_cbbc2506943b | 9/9 | 6 |
| hud | hud_gloss_84d394cb3fb1 | 61/61 | 11 |

Shipping由UEClient公开API执行BuildCookRun，通过，205.8秒，[结果](package-result.json)、[日志](package.log)。298项Source／Config／Resources／插件源文件绑定基线4db5789184fe38e041d62a1e68c8517338ea0b01和分支codex/TASK-053-title-wheel，[完整清单](source-manifest.json)、[版本](build-info.json)；源码未提交／推送／合并，未公开发行。

[包核对](archive-validation.json)通过：46项Resources与当前源码逐一一致，灰烬新图已包含，模型大小／SHA符合锁文件，包内不含开发Saved。Shipping执行文件SHA256：6b50dc3f24b0c83ec3ac7c9ba32f38d777f57209f8c5de8b5010bbefdd51c127。

## 安装、备份及启动

安装：C:/Users/22543/Desktop/Hearthward-20260929-9058ee2/Windows。

完整旧程序和玩家备份：C:/Users/22543/Desktop/Hearthward-20260929-9058ee2/备份-TASK053-20261003_latest_ui，包含Windows全部143文件、PlayerSaved下17个原SaveGames／Config文件，以及RootFiles下原说明／版本／启动入口。候选全部144文件先核对再换入；无递归删除。原桌面快捷方式及GPU／CPU入口字节一致，根BUILD-INFO与安装内相同；界面更新说明已更新。见[部署](deploy-result.json)、[独立核对](deployment-validation.json)、[最终保留](final-preservation.json)。

[安装后实际启动](shipping-startup-latest_ui.json)通过：PID22280，六次窗口观察均有窗口、标题正常并响应；31.99秒后停止自有进程。正式存档与GameUserSettings.ini在启动前后指纹相同，原17个备份文件最终恢复为相同字节。

Windows PowerShell5.1将无BOM的中文脚本按ANSI解码，首轮备份目录名异常；内容未丢失，目录已安全改为上述名称，脚本加入UTF-8 BOM。[修正记录](backup-path-correction.json)与原执行脚本保留。首次完整保留检查还发现UE启动清理了一个旧CrashReportClient.ini并生成新诊断文件；停止进程后已从备份恢复旧诊断，新增诊断保留，正式玩家数据始终未变。[原检查](final-preservation-initial.json)、[诊断恢复](diagnostic-restore.json)保留，不冒称首轮全部通过。

工具自测33/33通过，[结果](tool-tests.json)。[仓库自检与git diff --check](repository-checks.json)通过；[本地授权范围审计](scope-audit.json)通过，全部路径均在任务允许范围内。审计改用Git的NUL分隔UTF-8文件名，消除中文路径被引号转义造成的误判，首轮结果保存在scope-audit-initial.json。正式基线快照范围检查仍因TASK-053在基线没有获批任务快照而FAIL，不等同于本地用户同步授权。

## 验证边界

定向界面验证来自Development原生Widget／Controller事件与实际渲染；Shipping验证包含构建、源与资源一致性、完整安装、备份和实际程序窗口启动。未执行Shipping逐页物理键鼠或完整玩法实玩，Owner后续可从原桌面入口检查观感。
