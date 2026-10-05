# 局部收径候选实际渲染：拒绝正式绑定

2026-10-04，root串行UEClient，UE5.8.2；基线67fb0784ca8c6d488173e587e7f95c4be0d9092a及当前未提交批次。命令：`python -X utf8 docs/qa/TASK-055/run_stone_axe_pose.py --local-section-candidate`。实际CLI exit0，sampling_complete=true，error=null，编辑器关闭。

实际原石斧47,631顶点／95,278三角；10,446顶点的局部收径仅构建到`/Engine/Transient.Task055LocalSectionTransient`。临时源／渲染三角数均95,278，候选位置最大读回误差2.5228e-6cm。原source位置在build后立即恢复，最大误差0，包一直clean；未注册新资产、未保存包、未重建原网格渲染数据。3个socket位置保持原值。

真实SingleNode先source0初始化，再source .5500000119s；actual hand_r压缩姿态平移误差3.6140e-5cm、四元数误差3.7600e-7。世界scale .7，Grip对目标误差4.1583e-14cm，临时HeldMesh附着实际hand_r；指骨实际Transform保留在facts中。仅采当前姿态。

root检查full、palm-reverse、index-side三张实际PNG：握点附近成为明显细颈，宽大两端和斧头与1.140×.921cm柄径比例失衡。85%局部收径候选拒绝正式固化。离线205个已选皮肤面无非切向交叉、.243mm最小表面间隙，不能抵消实际形状问题，也不覆盖衣袖、其它动作、Native blend或弟弟。现有正式资产及生产Grip TF未替换。

实际图片和完整原记录：`Saved/Task055/stone-axe-local-section-candidate/`；可审阅facts：`stone-axe-local-section-actual-render-facts.json`。下一步只调查现有真实手指骨握姿修正，不继续盲目收细实体柄。正常输入、全动作持握和Owner观感／许可仍未验收。
