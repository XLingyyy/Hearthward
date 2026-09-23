# Tripo 敌人候选模型来源

- 生成日期：2026-09-23
- 输入：本目录 `短刀兵/`、`重甲兵/` 的 2 张参考 PNG，由仓库 Owner 提供；Owner 已授权本次公开仓库同步
- 服务：Tripo API；静态模型 `v3.1-20260211`，50,000 面上限、详细 PBR 纹理、四边面、按原图视角对齐
- 绑骨：Tripo `v2.5-20260210`，Rig Check 均返回 `riggable=true`、`rig_type=biped`；原生骨骼、FBX 输出
- 输出：2 个静态 FBX、2 张 PNG 预览、2 个带骨骼与蒙皮的 FBX；逐件任务 ID、文件路径和额度见[索引](敌人模型与骨骼索引.md)
- 额度：静态模型 45 credits/件，绑骨 25 credits/件，合计 140 credits

Owner 此前确认使用付费 Tripo 账户。Tripo 对[付费用户模型使用的说明](https://www.tripo3d.ai/help/privacy-policy/how-to-use-tripo-models-commercially)允许使用、修改、分发及商业利用输出，但输入图片的权利仍由使用者负责确认。本仓库不保存 API key、上传令牌、签名下载地址、本地生成脚本或运行清单。

本批是 `TEMP_VISUAL` 制作源资产，尚未导入 UE。FBX 中可见骨骼、蒙皮与绑定姿态数据，但未在 DCC/UE 中逐关节检查；短刀、锤、盾牌及厚甲的权重、穿插、动画重定向、尺寸、碰撞与材质仍需验证。候选外观不代表正式敌人兵种或文化设定。
