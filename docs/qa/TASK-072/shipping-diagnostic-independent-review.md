# TASK-072 Shipping 工程诊断独立复核

2026-10-05，只读复核 task051 的 package/new 原始证据及本地 UE 5.8 getter；未运行 UE、模型、构建，未修改 Source、Content、task metadata 或 Git。

**结论：当前 Shipping 工程构建及正常 new 的有限信用成立。** package-result 返回 dry_run=false/returncode=0，日志明确 Build、Cook、Stage、Archive 完成、BUILD SUCCESSFUL、AutomationTool ExitCode=0；Cook 0 error/0 warning。真实内层 exe 当前存在，169228800 字节。三个 Shipping junction 当前解析结果均与准备记录一致，属于该次 F 根；报告已明确保留 G 上 Binaries/PDB/receipt/UHT 写入，没有声称完全外置或正式 RC。

45 条 HTTP 事件顺序可核实：第 14 条 GetProjectSavedDirectory HTTP200、实际 own UserDir/Saved；第 18 条才 ExecuteAction new。随后实际 Natural world 引用、Loading true→false、HUD、一个玩家/一个伙伴及 AI gen0/server0 都吻合结果。22 条精确函数规则、AllowAny=false、child=false；未列入 IsDedicatedServer 返回明确“function not allowed”的 HTTP400。UserDir 路径及实际 Compatible-v8/pool.hws（169543 字节）位于独立池内。Shipping SaveTestPool 在 !UE_BUILD_SHIPPING 下，报告不计该参数隔离信用，正确。

launch 用真实 inner Shipping exe、实际 PID43772，无 ExecCmds；session ini 显式启用 HTTP。stop 原始结果只是 stop_ok，results 保留 NOT_VERIFIED_BY_UECLIENT，另附 Root exact-PID 缺席复核。没有 ExecuteAction quit 事件，报告不计正常/物理退出信用，正确。此次没有模型请求或 CSV，文件存在不能计手动保存/独立 Continue、性能、安装兼容或 TASK-074 发行验收通过。

WorldPresentation.cpp 第100/104行修复符合运行时接口：DirectionalLight.h 第46–52行 GetComponent 受 WITH_EDITORONLY_DATA 限制；Light.h 第85行 GetLightComponent 无该限制；Light.cpp 第225行固定 LightComponent0 类型为 UDirectionalLightComponent，第243行使用 GetLightComponent 赋予原编辑器引用。两个 getter 指向同一组件，Cast 类型成立；亮度、颜色、阴影和时间逻辑未改。未发现该修复引入的新缺陷。

证据保存有一项小范围编码限制：shipping-diagnostic-package-green.txt 与 F 根原 package.log 都在同一行含12个已写入的 U+FFFD。本次未输出该损坏行；阶段、Cook错误数、Exit0文本未受影响。应将其注明为日志编码残留，不以该行复核中文诊断，也无需因此重复构建。日志 BuildCookRun 自报360.83秒；文档“6分12秒”若为宿主总耗时，宜注明计时范围。
