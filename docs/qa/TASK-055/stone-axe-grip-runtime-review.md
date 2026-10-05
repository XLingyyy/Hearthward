# TASK-055｜腕部握点四姿态实拍

2026-10-04。root公开UEClient独立编辑器运行 --grip-candidate，已完成并退出；sampling_complete=true、error=null。Saved/Task055/stone-axe-grip-candidate保留4个实际compressed SingleNode姿态和8张PNG，full/grip已逐张核看。无Content资产保存、无生产动作或正常装备改动。

实际clip source0/.550000012/.733333349/.916666687。一次公开SetWorldLocation将测得Grip中心对齐真实hand_r腕pivot，后续不纠偏；四姿态最大误差约3.50e-14cm。实际parent socket scale约183.9176，.7 absoluteScale保留；UE求得Held相对平移(-.086199429,-.104666434,.068482885)cm。不能用Mesh世界scale1.839代替实际父socket scale。

真实15条右手手指骨已枚举并记录各姿态worldTF。实拍可见柄位于握拳/护腕附近，数学腕pivot对齐只证明稳定附着；腕pivot不等于掌心，不登记手指贴合或正常持握通过。仍需从真实掌部geometry及指骨测量候选掌心并实际近景复核。未测native graph0.12 blend、正常输入、敌人/墙首碰撞、护甲与耐久。

实际BladeBase/BladeTip候选为Mesh真实石刃两顶点，Grip为476个实际柄截面vertex的内部中心。后续socket作者仅保存该现有石斧包，先登记精确窗口与Owner LFS锁；不以候选新导入动作替换正式绑定或降低许可门槛。

官方socket作者依据：[StaticMeshSocket](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshSocket?application_version=5.6)，相对变换按真实parent应用，根本地5.8.2接口还须读回。

实际socket作者与独立编辑器重载均通过，author前Grip/BladeBase/BladeTip均不存在，author只保存既有石斧单包；probe无保存。两刃端点逐实际LOD0 vertex41603/26958核对，读盘后全部socket Outer/名称/坐标/Scale1正确；材质和0/0碰撞保持。Owner锁53719058保留。QA stone-axe-sockets-{author,probe}-{launch,results,stop}.json与Saved/Task055对应原报告保存真实结果。
