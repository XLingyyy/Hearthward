# TASK-072 正常Standalone公共引用接入

2026-10-05，Root串行公共UEClient，UE5.8.2 Editor Development -game，基线67fb0784ca8c6d488173e587e7f95c4be0d9092a上当前未提交补丁。独立UUID池1d8b6d41-3f2e-450f-aa59-7db42d2b1815与UserDir，临时loopback63289，actualPID37560。没有世界夹具、角色定位、物资授予、模型启动或模型请求。

正常Bootstrap World seed经实际Pawn.GetLevel→World核对，通过PC.GetHUD及public Screen READ_ACCESS得到Title；等待实际Loading.IsLoading=false后仅ExecuteAction(new)一次，正式换到L_HearthwardWilds并重取全部World-owned引用。实际一名玩家与一名伙伴，NaturalWorldEnabled=true、CampaignId有效，HUD及Loading结束，AI busy=false/ready=false/generation_calls=0/server_process_id=0。REFERENCE_SMOKE_PASS，host退出0、公共stop_ok，Root额外确认自有PID37560已消失。stop API本身不wait的边界仍保留原JSON，不由该字段冒称自行验证退出。

临时RemoteControl.ini位于本次Saved输出，21个精确ClassPath/FunctionName规则、childfalse、AllowAny=false；源码说明见remote-control-process-policy-source-review.md。generated SavedLayer的重复数组键无前导+，FunctionName用带双引号Optional文本。同一进程具体引用函数HTTP200，同类未列入的只读IsDedicatedServer返回明确function-not-allowed HTTP400。无Config目录写入或永久放宽。

历史真实失败分别留证：initial GET在引擎启动期间10秒超时，startup只读GET轮询改为在既有deadline内重试TimeoutError/URLError；默认远程函数名单拒绝IsValid；第一次引用成功后new被尚在Loading的生产guard拒绝，补等待，不改生产guard；Settings默认对象被引擎明确禁止远程访问，撤回readback；generated临时ini误用default层+键导致规则未进入属性，去掉+后通过。bare FunctionName请求曾HTTP200，但没有精确过滤负对照，不记作规则精确通过。全部原HTTP/启动/结果/stop按各自前缀保留。

此结果只打通当前正常Standalone采样的公共引用入口。性能、模型请求、UI绘制、全部OS、暖p95、五场景/两后端、Shipping、安装与第二机器均未由本项验收。设备采样前一次AC=1、电量100%、GPU59℃/12.94W，CPU温度及GPU功率限制未取得，见standalone-smoke-preflight-machine.json。

最终原始文件：standalone-reference-green-results.json、launch.json、stop.json、http-events.jsonl及RemoteControl.ini；各唯一运行输出仍在Saved/Task072。
