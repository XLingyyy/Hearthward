# TASK-053 · 行装管理返回背包修复（2026-10-04）

用户报告：点击行装管理左下“Esc 返回背包”或按Esc直接回到主世界。已修复为返回进入前的背包分类；背包继续按Esc才回到主世界。右上返回按钮也保留分类，菜单暂停开关、鼠标和焦点随返回页面恢复。

当前分支`codex/TASK-053-title-wheel`，HEAD `4db5789184fe38e041d62a1e68c8517338ea0b01`。源改动仍在工作树，测试与DLL指纹见下文；未提交、推送、合并或发布。

## 原因与修复

行装管理之前没有作为背包子页面记录到返回历史；左下按钮和Esc共用的back动作找不到上级，落到HUD默认值。此前177项检查只点了使用page:inventory的右上按钮，漏掉实际失败路径。

生产修改仅两处：OpenPage将背包→行装管理记为父子关系，保留原分类；back动作在行装管理明确指向背包，也覆盖没有父级记录的直接入口。原Esc处理仍优先取消确认框或拖动，其余页面返回规则保持。

## 当前证据

| 检查 | 结果 |
| --- | --- |
| 修复前复现 | [verify_0b233541](verify_0b233541/report.json)：307项中39项新增返回检查失败；原177项均通过。 |
| 修复后构建 | [build_06d02f11](build_06d02f11/build.json)：Development Editor Win64成功，零诊断。 |
| 修复后真实Slate回归 | [verify_8dc3c99c](verify_8dc3c99c/report.json)：307／307通过。 |
| UI输入及生命周期 | [equipment_return_final_65f97b26](../input-lifecycle-fix/equipment_return_final_65f97b26/report.json)：144／144通过。 |
| 复现与修复对比 | [red-green.json](red-green.json)：同一测试夹具，源码只变更上述两个UI文件，另有构建DLL变化。 |
| 当前源码／DLL绑定 | [fingerprint-checks.json](fingerprint-checks.json)：232与229项全部匹配当前工作树和DLL。 |
| 工具与仓库 | [repository-checks.json](repository-checks.json)：工具33／33、仓库0错误、差异空白及3599条本地授权路径审计通过。 |

24组返回组合覆盖四分类×左下鼠标／Esc／右上鼠标×菜单暂停开关；其中4组还经过设置页面再返回行装管理。每组检查实际进入、返回页及分类、暂停／鼠标／焦点、库存保持和后续Esc回世界。另检查直接打开行装管理时Esc仍回背包。使用FSlateApplication实际指针按下／释放及Esc按下／释放，未用直接page:inventory调用替代有问题的输入路径。此次307项同时重跑原177项装备腾格、拖放、存读档和行装管理操作回归。

环境、引擎和隔离配置见[environment.json](environment.json)及各轮launch.json。所有引擎操作使用显式本项目的GameFactory UEClient公开API；独立Development真实渲染1600×1000，TestClient/Runs独立配置和GUID存档池，人工测试档不被覆盖。没有执行Windows物理键鼠实玩或Shipping重新打包。

已目视核对[左下按钮返回装备页](verify_8dc3c99c/management-footer-returns-backpack.png)和[Esc返回原工具页](verify_8dc3c99c/management-escape-returns-tools.png)。修复前失败证据保留；初始8份文本快照及指纹位于[before/source-manifest.json](before/source-manifest.json)。

## 复验

双击[TestClient/启动测试端.cmd](../../../../TestClient/启动测试端.cmd)，Tab→选择任意分类→行装管理→点击左下返回或按Esc。应回到相同背包分类；再次Esc回到世界。

```text
python -X utf8 scripts/ui/verify_drag_client.py --suite equipment-return --build
python -X utf8 scripts/ui/verify_drag_client.py --suite equipment-return
python -X utf8 scripts/ui/verify_input_client.py --label equipment_return_final
python scripts/validate_repo.py
python -m unittest discover -s scripts/tests -v
python scripts/validate_repo.py --task TASK-053 --base 4db5789184fe38e041d62a1e68c8517338ea0b01
```

正式基线范围检查仍因基线没有TASK-053获批快照失败，本地allowed_paths审计单独记录，不替代该检查。README、TestClient说明和任务交接已同步。C盘安装保持删除，统一项目内测试端。此前道具／提示／原生玩法专项证据见[上一轮报告](../equipment-management/REPORT.md)，仅绑定各自当轮版本，不当作本轮重新运行的PASS。
