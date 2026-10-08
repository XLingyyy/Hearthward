# TASK-095 兄弟与族人服装接入

2026-10-08；UE 5.8.2、Blender 5.2.0 LTS、RTX 4060 Laptop。
范围基线 `0efbe05c099451bf1b6e031784af44c8b8afcd86`；完整受测实现提交 `cf3156de9d4cb650cd6dc2970614706369471fe9`。范围检查29路径、0错误，diff空白检查通过；工具自测沿用未变更工具代码的33/33通过记录。

沿DSGN-004批准的现有角色比例、粗布/皮革和色块方向，使用既有Hero/Brother制作源的ClothBlend衣料遮罩：主角棕色、弟弟灰绿、族人暖灰。遮罩1024²、线性数据，影响上限0.25；皮肤、头发和非衣料区域保留。原始高分辨率底色、法线、粗糙度和金属度图继续复用；没有重导入人体或改变骨架、身高、动画与手部挂点。

新增5包：3套材质、2张遮罩。材质从现有角色材质复制，仅在原BaseColor输出后插入遮罩混合；原骨骼用途保留。实际绑定覆盖主角、弟弟、Campaign族人和营地军需角色。内容写入前5包均取得本任务LFS锁，源Blend未保存修改。

## 验证

- Development Editor编译PASS；[build.json](build.json)。
- 定向Task095原生3/3 PASS、0 warning、0 error；[native.json](native.json)。
- 重开UE后的隔离PIE：6类实际角色网格加载成功，主角/弟弟/族人各自绑定指定材质，原网格路径与缩放断言PASS；[preview.json](preview.json)。
- 同光照下近景与六人同屏已实际渲染并检查：[兄弟与族人](brothers-civilian.png)、[六类角色](cast.png)、[三类敌人](enemy-roles.png)。颜色差异可见，人物未变形；该静态检查不代替连续运动、远距离/夜间或Owner视觉签收。
- 军需角色绑定入口已编译；本次截图使用Campaign族人，并未专门执行军需刷新流程。
- [导入记录](import.json)、[导入脚本](import_costumes.py)、[采样脚本](capture_costumes.py)。源遮罩脚本位于 `art_source/TASK-095/bake_cloth_mask.py`，从已有Hero.blend/Brother.blend逐份烘焙，不改变源文件。

首个LFS请求返回EOF，重试成功；没有在未取得锁时写Content。首次采样因Python不提供`get_relative_scale3d`方法停止，改为读取实际反射属性后完成。原始失败留在本地QA目录，未计入PASS。

TASK-095保持Active：其余武器持握、倒地/扶起、完整动作/远近昼夜验证、实际新版本Cook及Owner视觉仍未完成。族人继续复用弟弟网格，本轮完成衣料色块区分，没有声称制作独立人物拓扑或新服装几何。未合并main、未发布。
