# TASK-059 实际 Widget 渲染核看

2026-10-04。根通过 TASK-053/run_native.py --filter Hearthward.Camp059+Hearthward.Map064 --label render059064 --render --extra-arg=-Camp059Render --extra-arg=-Map064Render 执行，Native数据检查2/2通过，各有一个夹具初始化警告；不登记为正常键鼠或整个069验收。

Saved/Task059/native-widget-offscreen 三张1696×954 RGBA8实际Widget图已逐张查看：workers-normal-7-0、workers-severe-hunger-6-1、workers-offsite-4-0。五个身体格保持分配，主角的3.0／2.1／0.0实际工效使总量分别7.0／6.1／4.0；人员、按钮与页边界未见重叠或乱码。绘制方法和layout见同目录method.json及*-layout.json，明确C++生产Widget夹具。

同次064地图数据PASS，但两张实际地图截图侧栏描述与图例重叠；main05三行目标越过页脚。064视觉FAIL单列，不把combined数据PASS外推为视觉通过。原图保留于Saved/Task064/map-text-overlap-red，局部修复后只需重验受影响地图。
