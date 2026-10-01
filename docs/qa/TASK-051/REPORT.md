# TASK-051｜施工与定向验证

2026-10-02；Owner／Reviewer XLingyyy，无Issue。工作目录 G:/GameFactory/Hearthward/.agent-local/task051，分支 codex/TASK-051-experience-baseline。main来源 e1c44c49a88ce26125aa0e05fbe9d74b95ad7e58；路径授权快照 99c3c77193f6c6132e5d78a89db5e6f9a8d20bdf。用户已确认D1—D6并授权施工、提交和任务分支推送。源码提交及证据关联见本目录SOURCE.json。

已接入44个语义操作、主副键和单修饰键组合、上下文优先序、设备配置持久化、设置草稿／应用／取消、15秒显示回退、字幕及舒适性设置、实际胶囊攀越、自然湖游泳／溺亡和按最大生命值结算的坠落伤害。54个固定cue按实际信息、近距说话人和事件去重触发，缺失录音明确UNPRODUCED；没有动态TTS。大字号采用独立HUD布局及可滚动菜单。

| 验证 | 结果 | 实际范围与证据 |
|---|---|---|
| Editor Development构建 | PASS | UEClient.build.project，UE 5.8.2 / Win64 / HearthwardEditor；[build.json](build.json) |
| 定向原生 | PASS，7/7 | Experience 3项、Survival 2项、Combat 2项；0失败、0测试警告；[native-index.json](native-index.json)、[native.json](native.json) |
| 渲染PIE | PASS，51项检查 | 草稿、取消、INI保存、跨地图设置、暂停中显示回退、cue知识／距离／去重、攀越支出／落点／取消／顶棚／存档拒绝、正常新游戏、自然湖游泳／漂浮／消耗／下沉／真死亡和回档入口；[experience-pie.json](experience-pie.json) |
| Windows实际按键 | PASS，14项检查 | computer-use对实际PIE窗口按Y、Ctrl+U、I；实际INI和位移由脚本独立观察，4次I短按移动2.2756厘米；[input-pie.json](input-pie.json) |
| 大字号画面 | PASS，局部复核 | 150% HUD、48字号字幕、设置与键位页面；未宣称所有页面和字号组合均人工验收 |
| T-002、仓库L0、diff | PASS，0错误 | 相对批准快照检查全部修改和未跟踪路径；禁止路径负例只调用原验证函数 |
| Shipping、中文IME、32例逐项验收 | NOT_RUN | 本轮使用局部Editor构建及上述定向检查 |
| 五通道实际听音、人工录音 | NOT_RUN／UNPRODUCED | 音量及PCM16播放入口已实现，28组真人录音仍缺失 |
| D5联合性能、D6真人／十小时体验 | NOT_RUN | [PROTOCOL.md](PROTOCOL.md)和规划报告模板保留未测状态 |

环境：Windows 11 24H2、i7-13650HX、16 GB RAM、RTX 4060 Laptop、UE 5.8.2。原生使用NullRHI；渲染PIE使用D3D12、独立UserDir和随机测试存档池。攀越测试平台只存在PIE，标记PROTOTYPE_ONLY，不保存地图。自然湖检查通过正常新游戏加载L_HearthwardWilds；移动阶段使用Enhanced Input注入，实际键位另外验证。

首轮原生复现12米阈值浮点误差使角色保留极少生命值，修复阈值容差后通过，保留[native-first.json](native-first.json)。UEClient的native diagnostics包含首轮旧进程日志Condition failed；最终UE导出的native-index明确7个Success、0 errors、0 warnings，进程退出0。最终build.json对应全部C++修改；原生通过后新增的UI布局和箭回收筛选完成最终构建与渲染PIE复核，未重复执行不受影响的原生用例。

初次编辑器启动被本任务日志控制台QuickEdit选区阻塞；宿主仅关闭自己启动的UE进程的此控制台模式，后续启动成功，早期记录保留为experience-*-startup-blocked.json。实际输入初次无人值守启动超时；改用交互启动后捕获成功。单次I短按位移0.5689厘米，低于脚本1厘米判据，保留[input-short-tap.json](input-short-tap.json)，随后用多次实际短按复核。失败报告没有改写为通过。

ui-*.png：CaptureUI的1920×1080离屏UI，透明世界背景；pie-*.png：Shot SHOWUI的实际编辑器窗口及游戏视口，尺寸以图片为准。地图切换后离屏截图有字体图集局部缺字，时钟完整显示已用实际PIE图确认。离屏图不作为完整世界1080p渲染或性能证据。

从本worktree执行，Python使用G:/GameFactory/.venv/Scripts/python.exe -X utf8：

- docs/qa/TASK-051/run_build.py
- docs/qa/TASK-051/run_native.py
- docs/qa/TASK-051/run_engine.py --label experience --isolated-pool --timeout 540
- docs/qa/TASK-051/run_engine.py --label input --interactive --isolated-pool --script docs/qa/TASK-051/verify_input_pie.py --result Saved/Task051/input/results.json --timeout 400
- scripts/validate_repo.py --task TASK-051 --base 99c3c77193f6c6132e5d78a89db5e6f9a8d20bdf
- git diff --check

实际键位脚本通过ready.json请求操作，computer-use执行并观察后才写对应.sent；这些文件只同步阶段，PASS由实际INI和坐标决定。复跑前清除本测试input目录的旧.sent。

施工完成，状态Active等待Owner最终体验签收。人工录音、正式性能和真人验收继续按协议执行。未创建Issue、未合并main、未发布；原027工作区及其他UE工程保留。
