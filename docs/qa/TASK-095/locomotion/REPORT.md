# TASK-095 步态接触校准

2026-10-10；UE 5.8.2，Windows、RTX 4060 Laptop。受测基线 `003bc6cd51f336a0e41ec114048ba0242e0296bf` 加本次三个095源码文件修改。仅改变动作播放与走跑混合，不改变角色速度、攻击周期或伤害。

## 根因与修改

实测原始动画的脚步位移不匹配旧播放基准；走与跑片段还存在约0.19个归一化片段的左右脚相位差。按源片段长度与实际位移标定：主角走110、跑390cm/s，弟弟走100、跑350cm/s。按角色当前速度推进共同距离相位，跑步片段偏移0.81；保持SequencePlayer正常推进与动画通知。共用Brother动画代理的守卫/射手保持原行为。

参考 Epic 官方[Distance Matching](https://dev.epicgames.com/documentation/unreal-engine/distance-matching-in-unreal-engine?lang=en-US)的速度/距离匹配原则，在既有C++动画代理内实现，没有增加插件依赖。

## 实际结果

- Development Editor 构建成功。
- 原生4/4 Success，0 warning、0 error：LocomotionContact、WeaponInstancePresentation、FootContact.ActualClipDispatch、FootContact.GroundGuards。前一个新增用例覆盖主角/弟弟90、180、250、350、600cm/s的实际移动组件及动画。
- PIE 576次实际位置/骨骼采样、144帧图像；六档速度覆盖90–600cm/s。着地样本条件为脚高度距最低点3cm内且脚相对角色向后移动。指标为世界空间脚速度绝对值中位数，原始数据及算法保留；该指标不等同肉眼完全无滑动。
- 主角350cm/s：约238.2→17.9cm/s；主角600cm/s：212.6→58.7cm/s。弟弟180cm/s：78.1→11.1cm/s；600cm/s：28.9→26.3cm/s。所有速度下实际角色速度保持请求值。
- [接触画面](contact.jpg)显示走跑中左右脚交替、角色比例正常。

## 失败保留与范围

首轮原生夹具缺少GameInstance却让音画组件Tick，空引用；第二轮隔离World Tick没有推进新生成角色的移动组件，20项失败。夹具关闭无关音画Tick，显式推进真实CharacterMovement及SkeletalMesh后通过；没有放宽阈值或修改生产玩法。初次PIE手动相位遗漏旧同步组清理导致UE断言，已清理GroupName并复测通过，原始本地失败目录calibrated-02保留。

本报告覆盖平地步态、已有武器及脚步回归。完整剧情、复杂坡地连续观感和Owner主观验收不能由这些数据替代；其余095–098结果见各单工程收尾报告。候选13不包含本次修改。
