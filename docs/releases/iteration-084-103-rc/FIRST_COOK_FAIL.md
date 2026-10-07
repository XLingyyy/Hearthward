# 首次候选Cook失败

状态：**FIRST_COOK_FAIL / NOT_BUILT / NOT_PUBLISHED**。本次尝试CandidateID `iteration-084-103-20261007-1`，源码版本 `0.2.0-preview.20261007.1`，独立输出 `F:/HearthwardDemo/iteration-084-103-20261007-1`。原路径保留，不删除、不覆盖或重用来掩盖失败。

公开UEClient实际result为ok=false、returncode=25、artifacts=[]。完整原结果 `.agent-local/qa/TASK-103/package-20261007-1/result.json` 和原始 `package.log` 保留；精确错误/告警/行号/命令由 [FIRST_COOK_FAIL.json](../../qa/TASK-103/FIRST_COOK_FAIL.json) 引用，首尝试元数据见 [FIRST_ATTEMPT_BUILD_INFO.json](FIRST_ATTEMPT_BUILD_INFO.json)。本Agent没有改原log或执行构建。

|步骤/原证据|实际结论|
|---|---|
|log112 `Result: Succeeded`|Win64 Shipping C++编译成功；只证明该步骤。|
|log433/484|处理总包1637、Packages Remain=0；实际Cooked1630，平台跳过7。不能写成整个Cook通过。|
|log257/259，摘要710/711|AssetManager缺少GameFeatureData资产类型规则，两条实际error。|
|log712/714|MCP插件EULA提示按原文留证据；最终2 errors/1 warning，不隐去或作法律推断。|
|log727—732|Commandlet ExitCode1，Cook failed，AutomationTool UAT25／BUILD FAILED。|
|第一输出实际存在检查|F目录存在，Windows/Hearthward.exe不存在；没有成功运行树或可交付exe。|

缺规则由原日志确认；root/MCP agent定位当前启用AllToolsets引入GameFeatures依赖，配置最小微修由其处理。当前uproject明确有AllToolsets/MCPClientToolset/ModelContextProtocol等条目。本资料不把依赖归属冒充原日志直接列出，也不改Config/Source/插件。

禁止用IgnoreCookErrors、删旧插件、移除日志error或把Native15/15成功写成包成功。root当前已授权MCP配置的必要微修；不更改Campaign/Save/玩法、模型锁或版本Header。

后续第二次CandidateID `iteration-084-103-20261007-2` 已在独立输出 `F:/HearthwardDemo/iteration-084-103-20261007-2` 实际 **BUILD_SUCCESS**，产品版本保持 `0.2.0-preview.20261007.1`。root新增GameFeatureData空扫描目录/AlwaysCook规则，第二次冻结patch/未跟踪清单已保存；公开result ok=true/ready/Exit0，Cook0errors/1warning，BuildCookRun124.78秒，证据见 [PACKAGE_BUILD_SUCCESS.json](../../qa/TASK-103/PACKAGE_BUILD_SUCCESS.json)。本记录及首FAIL JSON保持首尝试事实，第二次最终清单/独立操作仍待，本Agent未复制文件到F运行树。

随后第二次Shipping已有标题版本/卧室/J日志/F6手动保存/退出跨进程继续的部分正常输入证据；连续路线/模型闭合/联合性能/Owner/真人/二机/远端发布仍未验或未执行，见当前QA。编译成功与已处理包数不改变这些门槛。
