# 收集要素连续列表与根目录测试游戏更新

2026-10-04，分支 `codex/TASK-053-title-wheel`，HEAD `4db5789184fe38e041d62a1e68c8517338ea0b01`，Owner XLingyyy。用户要求去掉“收集要素”中物品之间的空行；延续同一项目根目录 TestClient 更新范围。

收集列表在排列前按原 `collected:物品ID` 历史记录过滤未收集物品。鼠标行位、滚轮、翻页、Home／End和上下选择共用这一连续列表。顶部仍显示“已记录 11 / 77”；页码变为“1 – 8 / 11”，只统计已记录物品。库存为零的历史记录仍显示。其他日志分类的未知行与原知识条件保持，收集、库存、任务奖励和保存格式未改。

实际产品改动仅 `HearthwardScreenContent.cpp` 两处；同一原生日志夹具更新收集分类预期，并新增稀疏11条记录、空列表、连续行位置、鼠标命中和大字号分页检查。6份原文本快照和当前61份人工Profile指纹保存在本目录。进入测试前的客户端状态亦保留，仅进程号随后由正常入口更新，Profile与save_pool保持。

| 验证 | 实际结果 |
| --- | --- |
| UEClient公开API，HearthwardEditor / Win64 / Development | `build.json` 成功，返回码0、无诊断；230份编译输入绑定 |
| 实际PIE日志与生命周期 | `verify_38fb5a93` 124/124，18张原生UI PNG |
| 收集显示／选择／翻页 | 0条、11条稀疏记录、8行首尾页、鼠标选择箭矢及原24件数量、库存归零保留、动态新增记录均通过 |
| 150%字号和画幅 | 每页6条连续记录，首尾范围正确；1672×941、1280×720、2560×1600、2560×1080截图；普通、大字号、720p和超宽画面已目视核对 |
| 正常根CMD进入的独立游戏输入 | `verify-normal` 144/144，包括J、M及其他UI快捷键、暂停／返回、登录、新游戏／继续／读档 |
| 实际默认根入口 | `root-smoke.json` 11/11，PID9836，登录加载完成、三次Development窗口响应；观察后窗口保留 |
| 当前程序／资源 | 日志验证与正常游戏各282项指纹，根入口282项，编译输入230项均绑定当前文件 |
| 原人工测试资料 | 隔离验证后61份原文件未改；实际正常入口观察时17份Config／SaveGames文件逐字节保持，原Profile与存档池相同 |
| 仓库／工具／授权增量 | 见 `delivery-integrity.json`、`repo-validation.txt`、`tool-tests.txt`、`diff-check.txt`、`incremental-scope.json` |

相对前轮根游戏的282个运行输入，只改变本次Content.cpp、JournalTest.inl及重新编译的DLL，另外279个输入相同。地图水色、折角、暗烟雾、火焰及其余UI素材均保持原文件。地图专门的56／29／12项证据是历史版本的测试；当前游戏以本轮编译及指纹为准。

首轮 `verify_3463af4f` 的收集相关检查通过，但Bootstrap尚未初始化试玩进度，原领奖与返回HUD两项失败。后续 `verify_f7b97aa3` 为Python调用了不存在的get_pawn，`verify_683b1a8b` 为没有创建保存夹具所需伙伴，均在正式检查前终止；补齐原既有测试的伙伴、冒险和Prototype初始化后，最终124项全部通过，没有删除或跳过检查。所有失败报告、日志及自有进程停止结果保留。

首次普通根窗口PID45044已加载，10项检查通过，但三次窗口采样未取得；日志随后出现ConsoleCtrl退出。记录保存在 `root-first-observation/`。采样改为UTF-8并保留原始观察值，紧随根入口启动重新观察，最终PID9836三次响应均正常。此前根目录接入的尾部资料审计因UE在普通游戏退出后重写EditorPerProjectUserSettings.ini而失败，原真实游戏存档／设置及备份均保持；该旧失败没有作为本轮成功证据。本轮以现有资料的新快照和17项启动核验为准。

仓库正式基线范围检查仍因HEAD没有获批的TASK-053任务快照而FAIL；当前用户授权的6份快照增量及QA目录另行核验，不伪称获批基线通过。未提交、推送、合并、Shipping打包或公开发布。Windows物理键鼠试玩与Owner视觉验收仍由用户进行。

双击 [启动测试版游戏.cmd](../../../../启动测试版游戏.cmd)，进入或继续游戏后按J，再选择顶部最右侧“收集要素”符号。原TestClient入口继续可用。

![普通字号的连续收集列表](verify_38fb5a93/verify_38fb5a93-journal-collection-compact.png)

复验：在Hearthward根目录先运行 `python -X utf8 docs/qa/TASK-053/journal-compact/build.py`，成功后运行同目录 `verify.py`；正常根入口可传 `--verify-input` 加新的绝对report.json路径，脚本拒绝已存在报告。
