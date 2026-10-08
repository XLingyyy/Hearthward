# TASK-096 近景材质与门框增量

2026-10-08，UE 5.8.2 / Blender 5.2.0 LTS，RTX 4060 Laptop 8GB。分支codex/TASK-084-103-iteration，准确范围基线71ef41c44f3a02f3481f8192d385179bb8960560。受测实现提交：5638ddf4723697c0006fadf75bce99016e2595cc。

2026-10-08 近景增量：原创灰石/旧木受光材质、四张1024贴图与2700三角面卧室门框已接入，新增七个准确资产包。门框净宽280厘米、净高380厘米，仅装饰；原碰撞和导航不变。Editor构建及卧室/撤离净空原生1/1通过，正式地图隔离新档的三个游戏视口已检查。夜间卧室入口与回廊地面可辨，梁下和回廊门框背侧仍较暗；没有据离屏截图调亮正式灯光。完整火烟、连续路线、新版Cook与Owner视觉验收未完成，TASK-096保持Active。

## 制作与绑定

原创周期噪声灰石、旧木色彩/法线贴图各1024×1024。材质使用引擎WorldAlignedTexture/WorldAlignedNormal，80厘米世界尺度、粗糙度0.91；法线采用Normal采样器、翻转绿色通道、WorldSpace静态布尔输入和非切线输出。门框25块倒角石块合并2700三角面，外框404×112×423.2厘米，零碰撞；原生墙柱和顶梁保留，材质应用到原Stone/Timber分件。CDO硬引用新材质和门框，未修改地图。

制作源art_source/TASK-096/Nearfield.blend及author_nearfield.py，原创几何及数值纹理，无下载图像。所有七个uasset和blend编辑前已取得LFS锁，保留至整合。author.json记录源尺寸和法线约定。Blender前后左右上五视图已检查。

## 实际验证

build.json：Editor构建成功。native-index.json：Hearthward.Hometown077.BedroomAndEscapeClearance 1/1成功，测试零警告/零错误。测试只覆盖原生净空，不能代替连续游戏通关。

preview.json：正式Natural/Rebuild地图，隔离存档，真实UI new动作，生产HearthwardGameMode；新档时钟1201.4104分钟。唯一石堡实例、门框挂接和NoCollision、两种材质各引用两张纹理断言通过。地图GameMode覆盖仅在未保存的PIE准备场景，不修改主地图。公开UEClient负责启动和关闭。

frames/viewport-night-*：游戏视口Shot截图，临时CameraActor仅改变观测视角。卧室正侧面倒角接缝可见，没有发现缺面或浮空；回廊地板可辨。回廊门框背侧、梁下阴影仍暗，完整舒适度和昼夜/局部火光不计PASS。actual-player-night为之前原玩家相机视图。没有运行正常键鼠连续撤离、Cook或Owner签收。

## 失败与修复

首次材质法线使用默认TextureObject采样器导致SM6编译失败。改Normal采样器和WorldAlignedNormal的WorldSpace输入后，在正常RHI重开确认两张纹理引用及正常显示。初次import.json只提供结构导入结果，不能当作当时的材质渲染成功。

SceneCapture离屏夜间明显比实际视口暗。diagnostic/night-bedroom保留该对照；提高灯光的候选导致实际视口过亮，未写生产配置。diagnostic-day-bedroom为未保存场景中临时日光、候选灯光的诊断，不代表正式昼间。最终灯强、位置和时钟算法均保持原值。离屏与视口差异的具体渲染原因未确认。

复现脚本保留在本目录，输出仍写.agent-local/qa/TASK-096。早期Python属性调用和回调重入错误已修正，未计为测试成功。完整TASK-096保持Active。

截图采用Epic官方Shot与SetViewTargetWithBlend路径：https://dev.epicgames.com/documentation/en-us/unreal-engine/taking-screenshots-in-unreal-engine 。

范围检查35条路径、零错误；git diff --check通过。八项LFS锁归XLingyyy，保留至集成。
