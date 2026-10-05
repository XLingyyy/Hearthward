# TASK-069 侧栏／画布滚轮分支最窄Native验证

2026-10-04。`map-sidebar-wheel-validation.patch`相对根正在集成的End／Home增量后版本，仅在既有125／150同一fixture的End捕获之后补滚轮路由验证；原100%全文和fixture初始化不改。应用顺序：production3cpp、已有map-sidebar-scroll-validation.patch，再本次wheel增量。

使用本机UE5.8 Geometry.h:197公开FGeometry.MakeRoot创建1696×954的真实Slate geometry，布局变换显式scale1.25和offset40,30。依据同Widget设计1672×941的实际paint scale／居中offset，通过Geometry.LocalToAbsolute把真实sidebar与canvas矩形中心转为screen-space坐标。使用本机Events.h:721–746的公开7参数FPointerEvent，保持按钮集合为空、EffectingButton Invalid、wheel delta +1。没有默认零尺寸Geometry，没有私有字段／测试setter／模拟变更MapZoom或TextScroll。

每scale在End位置取DescribeLayout的实际mapTerrain矩形与说明矩形：

1. 指针位于sidebar，+1滚轮须使说明Y产生向内容上部阅读的位移，同时terrain四项矩形值完全保持。
2. 指针位于canvas，+1滚轮须增大terrain宽／高，同时sidebar说明四项矩形值完全保持。
3. canvas -1撤回该次缩放，再沿既有Home恢复说明原位置，为下一个scale保留相同起点。

仅增加真实新增分支的断言，不增加截图或全页矩阵。原head及End截图仍保留。此方式为Native回调及明确geometry／pointer fixture验证，不能代替Windows设备缩放、OS鼠标或PIE实际输入验收。

本地完成API签名及局部结构检查；未UE／build／Git、未改root文件或生产源码。真实Native结果由根串行执行。
