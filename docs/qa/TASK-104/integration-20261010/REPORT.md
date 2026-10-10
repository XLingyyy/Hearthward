# TASK-099 / TASK-104 本机整合记录

日期：2026-10-10。当前分支 `codex/TASK-084-103-iteration`，基线 HEAD `bcbb62158b9ab7c977e809cc5b3084386ef632a8` 加未提交工作树。整合前工作树干净；已 fetch 的 `origin/main@1bdc01b642dcf6e322ddca6cd4d7ad1a3fcc6351` 与基线文件树一致。

## 输入与整合

用户直接提供 `Hearthward-TASK099-Audio-Patch-20261010.zip` 和 `Hearthward-TASK-104.zip`，授权本地整合。099包基线为上述main，104包基线为 `408b03ed175b096fedacfbc8845d3d5a82852a05`。包内历史推送/PR授权、云端403和离线测试状态作为历史记录保留，不扩展本次授权。本轮未提交、推送、合并main或发布。

- 按各包基线三方整合，保留当前步态相位与位移校准。合并[记录](merge-results.json)保留131项输入及冲突处理线索。
- 099接入落地、入水、出水、活跃火焰循环和山脊/瞭望点局部风声；保留包中第三版入水、减6dB风及第二版出水的交付音频。
- 104接入草、土、石、木及未知回退，共15份脚步WAV，每类3变体；使用真实接触分类并避免同类连续重复。共享斧头挂接与两角色专用握姿已接入。
- 合并后的 `experience.json` 为26个声音事件、31个不同引用WAV，引用文件齐全。
- 修复弟弟源码的相对include路径；修复原生物理材质测试的初始化顺序。Chaos创建solver时复制主材质表，测试需在创建世界前注册物理材质。测试仍检查真实碰撞与物理材质优先级，没有绕过断言或改变生产判定。
- 音频配置保护检查允许且严格核验104的5类/每类3变体，再比较其余历史字段。104 UE检查脚本使用渲染环境和独立存档池，不以NullRHI运行新增火焰测试。

## 斧头资产与实际画面

已核对本人XLingyyy的既有LFS锁 `53719058`，只保存 `Content/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.uasset`。原包备份在本地私有整合目录，锁仍保留。

[应用结果](axe-apply.json)：顶点差量最大误差0，拓扑/UV、材质、Grip/BladeBase/BladeTip插槽保持；现有95278个渲染三角形未作性能重构。[重开编辑器只读检查](axe-probe.json)确认 `already_applied=true`。没有保存地图或修改其他Content资产。

在隔离的临时灰盒PIE中使用正式主角/弟弟与库存装备路径，共采样32帧待机/行走，见[采样结果](visual-result.json)、[预览视频](grip-preview.mp4)、[主角待机](Hero-idle-03.png)、[主角行走](Hero-walk-03.png)、[弟弟待机](Brother-idle-03.png)、[弟弟行走](Brother-walk-03.png)。代表帧确认细柄与握姿已进入游戏渲染。灰盒灯光偏硬且手套较暗，主角小指较松的限制保留；不据此认定全部战斗/采集动作或主观视觉验收通过。

## 已执行验证

| 检查 | 结果 | 证据 |
| --- | --- | --- |
| UE 5.8.2 Development Editor | PASS；首次include错误已修复 | [首次构建](build-01.json)、[修复后构建](build-02.json)、[最终构建](build-04.json) |
| 099原生音频 | 30/30 PASS，含7项新增移动/风/真实Niagara火焰测试 | [联合首轮40项](native-01-index.json) |
| 104原生握姿/脚步 | 9项均已有PASS结果；首轮8/9，初始化修复后剩余单项1/1 | [首轮](native-01-index.json)、[中间复现](native-02-index.json)、[最终单项](native-03-index.json) |
| 095步态接触回归 | 1/1 PASS | [联合首轮](native-01-index.json) |
| 104 Python工具/导入器 | 32/32 PASS | `python -m unittest scripts.tests.test_axe_handle_grip scripts.tests.test_surface_footsteps scripts.tests.test_task104_axe_import -q` |
| 099音频配置与素材引用 | 6/6 PASS | `python docs/qa/TASK-099/audio-completion-v1/validate_audio_completion.py` |

原生结果是首轮39/40加最终单项1/1，没有声称一次性40/40。最终只改变失败测试的初始化，因此不重复已通过的39项。首轮保留两条warning：RHI保留虚拟地址超过256GB预算，以及故意缺失WAV的回退测试警告；最终单项0 warning/0 error。原生使用真实渲染环境，但运行器的NoSound结果不代表设备实际发声或听感验收。

仓库链接/任务快照检查及差异空白检查的最终结果另存 [repository-validation.txt](repository-validation.txt) 和 [diff-check.txt](diff-check.txt)。

## 尚未覆盖

实际设备声音、正式地图连续跨地面与水/火/风混音、完整战斗/采集持斧表现，以及全流程体验仍未验收。没有重新Cook/Shipping；候选14独立程序仍是原版本，本次内容可由当前已编译Editor工程运行。TASK-099和TASK-104保持Active，不能据本次局部验证标记整批084—103完成。
