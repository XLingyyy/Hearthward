# Cooked 火烟加载时序修复

2026-10-08。范围基线 c7d1db75；实现完整SHA在提交后绑定。

候选6的Shipping构建成功，但实际启动黑屏无响应。通过本机已有Windows SDK的DbgHelp读取自有进程线程栈，确认GameThread进入ReportCrash，异常链为UNiagaraStatelessEmitter::Serialize ← FObjectFinder<UNiagaraSystem> ← 石堡构造函数 ← UClass::CreateDefaultObject ← PreInit。工具仅取线程上下文与栈，逐线程恢复，没有改写进程状态或引擎文件。

UE5.8的NiagaraStatelessEmitterTemplate.cpp在模块尚未启动时延迟InitModulesAndAttributes；NiagaraStatelessEmitter.cpp的Cooked加载分支则按模板模块列表恢复被裁剪模块。原石堡CDO同步加载让这两段发生在错误顺序。Epic的[历史初始化问题](https://issues.unrealengine.com/issue/UE-71147)提供软引用方案参考；具体本次定位以本机栈与5.8源码为依据。

两个系统改为UPROPERTY软引用，仅在序章附近首次创建火点时同步加载。三个火点、颜色、密度、尺寸、碰撞、伤害、距离门槛和清理流程保持。候选7随后暴露Cook未收录软引用目录，实际IoStore清单中Fire条目为0；因此只把精确TASK-096/Fire目录加入已有DirectoriesToAlwaysCook清单。未增加全目录Cook或引擎补丁。

候选8 Build/Cook/Stage/Archive通过，Cook 0错误、1条MCP许可提示；实际容器包含全部5个Fire包。Computer Use在独立UserDir启动Shipping，通过鼠标进入新游戏、F6打开保存页、保存1→2、选择手动节点并确认读档，恢复卧室和实时HUD。1280×720窗口模式实际应用，未及时完成确认后自动回到原显示设置。未把该操作计作已保留设置、拖动缩放或完整撤离验证。

证据：[cooked-loading-fix.json](cooked-loading-fix.json)。原失败候选6/7、原始包日志、原生栈和屏幕证据保留。独立Shipping火烟近景与完整路线仍待验收；Editor首件视觉与原生命周期报告保持各自版本范围。

局部原生回归 `Hearthward.Hometown077.BedroomAndEscapeClearance` 1/1成功，测试内0错误/0警告，覆盖火点数量、距离/阶段和销毁清理。进程启动仍有13条Condition failed及驱动、MCP许可、SSGI提示，单列于JSON，不计作全进程零错误。原始目标报告见[cooked-fix-native.json](cooked-fix-native.json)。
