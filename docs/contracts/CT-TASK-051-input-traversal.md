# CT-TASK-051｜语义输入、设备偏好与通行表现

APPROVED：Owner／Reviewer XLingyyy于2026-10-01批准D1—D6，并授权沿051实施、提交推送。规则依据为[DSGN-R23](../design/DSGN-R23-input-traversal-acceptance.md)。无Issue；设计批准与最终体验签收分别记录。

## 输入与页面

`HearthwardInput::Definitions()`是44项可绑定动作的唯一目录。每项保留主／副键，支持键鼠单键及一个Shift／Ctrl／Alt修饰键；设备存储使用稳定动作ID和物理FKey名称。相同上下文的重复绑定阻止应用，已登记的R优先族、E交互族和Shift修饰族允许共享。Esc、Enter及系统快捷键保留；控制器不在本次交付范围。

Character建立Enhanced Input映射并按当前语义消费世界输入；页面、钓鱼和HUD提示读取同一绑定。处决可行时消费R，随后才是仓储与弩装填；交互沿救援、搬尸、回收箭、工作台、剧情、采集、攀越的顺序。打开页面、读档及应用失焦释放已按输入，防止松键发射或冲刺继续。文本框由Slate处理IME和提交；危险确认没有默认Enter执行。

## 设备偏好与显示恢复

`UHearthwardPlayerSettings`为GameInstance子系统，管理绑定和字幕／文字／音量／舒适性偏好，写入GameUserSettings配置。设备偏好不进入世界存档，无schema升级。设置草稿只在应用时提交；退出放弃草稿。显示模式或分辨率变化开启15秒实时时钟确认，超时／取消只恢复显示模式和分辨率，其他已应用偏好保留。

125%／150%文字采用可滚动正文与按钮布局；方向键、Tab及滚动键使按钮可达，页面返回按稳定action恢复焦点。音量使用主音量与音轨比例的乘积；音频组件用`Hearthward.Audio.music/voice/environment`标签分类，未标记组件为效果。

## 通行、伤害与时间线

`UHearthwardTraversalComponent`与自定义CharacterMovement负责低障和水区。攀越只接受地面起步、45—120 cm高度、80 cm前探、60 cm可站立落点及完整胶囊净空；开始支付8耐力，1秒内沿扫掠路径移动。伤害、移动、Esc、阻挡和epoch改变取消；保留实际位置与费用，不写动作快照，过程中禁止保存。

自然西湖的既有中心、半径和157 m水面登记于`Resources/Data/experience.json`；实际地面可站立时保持步行，其余水中输入速度300 cm/s。正耐力静止浮水无费用与恢复；主动游泳按MaxStamina/25及既有负重／效果修正一次。耗尽时开始10有效实玩秒溺水，短暂出水不重置，能呼吸并站立上岸才重置。暂停继续沿044冻结世界时钟。

落地按实际重力与下降速度换算等效落差：3 m以下无伤，3—12 m线性到MaxHealth，12 m及以上封顶。浮点阈值使用UE既有容差，环境伤害交生存系统，不套护甲；地面致零保留倒地救援，溺水为真死亡。

## 固定对白与可读表现

`UHearthwardPresentationComponent`使用54个cue及事件GUID防重复，要求实际附近说话人和已知事件；死亡、取消、读档清理旧显示／声音。固定PCM16 WAV按28个audio_group从`Resources/Audio/Fixed/`加载，缺录音明确标记UNPRODUCED，仍显示获准字幕；动态回复继续文字。没有新增TTS、云调用或人声伪造。

字幕与HUD独立于声音。严重饥饿的边缘遮罩、远景投影和FOV按同一舒适性强度调整，0%仅移除视觉限制；权威伤害、饥饿处罚、AI感知与地图知识沿原系统。

## 验证边界

运行覆盖以[051报告](../qa/TASK-051/REPORT.md)为准。自动化与PIE不代替物理键盘／IME、所有路线、真人试玩、联合模型性能或正式人声验收；这些门槛保留在已批准协议中。
