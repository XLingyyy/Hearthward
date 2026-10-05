# 物理键鼠路线独立证据审阅

读取 Root 真实目录 `Saved/Task071/physical-8618582c-c61b-4686-b19f-0e80f6f8a7ba` 的 setup/progress/events/layouts/results/host-result。未运行 UE、构建或 Git，未修改 Root。逐项结果及原始引用保存在同目录 `physical-os-route-independent-evidence.json`。

实际 Editor PID48088，host `CAPTURED`、`recording_complete=true`、`stop_ok=true`。21项夹具检查均true；共1451次采样、6条状态变化、230条布局变化，覆盖约726.45秒。采样器和Owner验收字段仍为 `NOT_EVALUATED`，正常新游戏为 `NOT_TESTED`。

| 节点 | 独立记录支持的事实 |
| --- | --- |
| baseline A / future | A的玩家木材0；公开TryAdd形成未保存future木材1，同epoch/存档节点。来源80、仓库木材20、弟弟木材0，请求/交付计数0。 |
| seq215 / sample417 | 页从HUD变dialogue，真实 `dialogue.input` 可见rect[960,766,510,52]；后续采样继续，无authority/epoch/model变化。 |
| seq216 / sample1298 | dialogue回HUD，future库存与epoch保留。 |
| seq225 / sample1314 | 页变save，手动A动作 `ask:load:A57D1509407B4332B21641BF92E7C916`，rect[475,373,390,61]，与baseline保存ID对应。 |
| seq226 / sample1356 | 首次出现确认modal，confirm rect[550,510,240,55]、cancel rect[850,510,240,55]；future库存和epoch保留。 |
| seq227 / sample1400 | modal移除，仍在save页；首次modal持续约22.004秒期间没有采样到layout、authority或epoch变化。取消后仍为玩家木材1、同epoch。 |
| seq228 / sample1420 | 相同确认modal再次出现，仍保留future。 |
| seq229 / sample1449 | save回HUD；唯一一次观测epoch更替；玩家木材1→0，状态为“世界与知识已恢复；旧时间线请求已废止”。最终整个被捕获authority与A完全相等，campaign ID保持。 |

最终玩家/弟弟/来源/仓库木材分别0/0/80/20，请求和交付计数0。此前全部状态事件没有改变authority；model generation和server PID观测始终0。Host成功停止自身PID，日志未出现采样器error。当前证据支持危险modal保持、取消保留future、一次实际载入恢复A的被捕获库存/命令计数状态。

sample1199新增一份 `manual=false` 自动保存节点，当时页仍dialogue，发生在存档页打开之前。它不能归因于F6打开save页，也未被当作手动A。最终保存列表含三节点，只有A为manual。

独立子审阅确认两次modal布局完全相同，取消后的save布局完全恢复到打开modal前；modal panel rect[490,310,690,290]。modal记录没有待执行动作GUID，因此Confirm与手动A按钮点击的绑定需要Root OS记录佐证；最终authority相等支持恢复了与A一致的被捕获状态。

Root报告实际操作为T、鼠标焦点与type_text中文/E/R、Escape、F6、鼠标选A、Return、Escape、重新鼠标选A与Confirm。状态/布局记录与这些结果吻合；采样器没有记录键值、鼠标事件、Native Draft文本或焦点，不能独立证明这些输入的来源。`DescribeLayout`显示输入框和modal的预期可见布局；首次Paint、中文字形与实际屏幕呈现需要Root的物理输入/截图证据关联。type_text保留其实际方法，IME未覆盖。

保留prototype夹具披露：临时地板、移位CombatTarget、SourceSafe、物资/workbench、disabled Gameplay tick与独立测试存档池。没有正常新游戏、正式路线、修饰键矩阵、九页矩阵、IME、Owner视觉验收或完整世界存档域信用。0.5秒采样可能漏过短暂状态；监控不主动判定整体PASS。
