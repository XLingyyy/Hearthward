# 草簇矩形接触阴影修复

2026-10-08，受测基线及环境见[binding.json](binding.json)。两个草种资产在新Editor进程加载后均为CastContactShadow=false，真实Landscape草组件采用该值；正午原机位中的近处矩形黑斑消失。全局r.ContactShadows仍为1，建筑与树木阴影保留。

![修复前](before.png)

![重新启动后](after.png)

故障复现于石堡庭院：遮罩草片下出现与卡片对应的实心矩形。先分别关闭接触阴影、距离场阴影和距离场AO；只有接触阴影对照去除了该现象。随后只对草组件调用SetCastContactShadow(false)，再次消除近处黑斑。已有草材质为Masked/TwoSidedFoliage，动态阴影和距离场影响原本关闭。

修复保存于GT_Meadow和GT_S1_Meadow两个LandscapeGrassType的GrassVarieties。没有改光源、全局画质、密度、材质、碰撞或主地图。资产已有XLingyyy锁51998986与52010301，保留到集成。制作脚本见[fix_grass_contact_shadows.py](../../../../art_source/TASK-097/fix_grass_contact_shadows.py)。引擎LandscapeGrass.cpp按照草种属性生成组件，因此后续流送/生成的草簇也读取资产设置。

首次保存未标记包修改，save_loaded_asset默认只保存dirty包，函数返回成功但重开后的值仍为true；[失败记录](first-persistence-failure.json)保留。脚本已显式modify、写回结构数组并强制保存这两个准确包；[重新启动验证](restart-verification.json)同时检查磁盘加载值、PIE组件值、全局开关和实际图像。

验证使用独立档池、真实新游戏入口和临时观察相机。时钟加速到正午后恢复1倍速，画面时刻约12:01—12:12。该夹具的加速移动警告及原有启动警告不计入本局部图像通过，也不宣称无警告。没有运行重复编译或不相关原生套件：本次只改两个现有资产属性。

候选8在本修复之前生成，尚未重新Cook此改动。此记录不提供完整自然路线、性能、Shipping或Owner视觉验收信用。

技术依据：[Epic接触阴影文档](https://dev.epicgames.com/documentation/unreal-engine/contact-shadows-in-unreal-engine?lang=en-US)说明该效果按屏幕深度执行光线步进；具体草种控制路径已同时核对本机UE 5.8.2的LandscapeGrassType.h和LandscapeGrass.cpp。
