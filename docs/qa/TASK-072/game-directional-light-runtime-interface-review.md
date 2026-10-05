# TASK-072 Game编译首错与两行替换等价性

Root首次Development package在Compile结束，ExitCode6/OtherCompilationError，Cook未启动。首个C2039来自HearthwardWorldPresentation.cpp100、104调用ADirectionalLight::GetComponent；C3536为100行的后续错误。没有Disk失败，末次只读Gfree约2.245GiB。外层UAT日志已包含replacement字节；Root已归档原正确UTF8 compile-red.txt，本稿不重复损坏中文。

独立源码核对确认Root当前两行替换等价：

- ADirectionalLight的GetComponent定义位于DirectionalLight.h46–52的WITH_EDITORONLY_DATA内，不能用于独立Game。ALight::GetLightComponent位于Light.h85，无editor条件，为真实运行时light对象。
- 本机Engine/Private/Light.cpp224–225构造时以UDirectionalLightComponent替换LightComponent0；243又把同一GetLightComponent CastChecked到editor专用DirectionalLightComponent。因此100行Cast<UDirectionalLightComponent>(Fill->GetLightComponent())取得原GetComponent指向的同一组件，保留SetAtmosphereSunLight等专用setter。
- 104行SetIntensity是runtime LightComponent支持的原有操作，改为NightFill->GetLightComponent()->SetIntensity不改变参数、对象或光照时间逻辑。

Root精准两行修复不需要Header、测试setter、fallback或Target改动；Editor与Game都沿既有运行时组件。当前重试package-green-attempt.log由Root执行，子代理没有启动构建或修改Source。此审阅只确认接口/对象等价，编译GREEN、Cook成功和实际Game光照仍以Root后续真实结果为准。

源证据：

- [DirectionalLight.h](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Engine/Classes/Engine/DirectionalLight.h:46)
- [Light.h](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Engine/Classes/Engine/Light.h:85)
- [Light.cpp](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Engine/Private/Light.cpp:224)
- [Root当前生产](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp:100)
