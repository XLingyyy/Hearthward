# TASK-013 交接

更新：2026-09-18。Owner：XLingyyy；分支：`codex/TASK-013-local-qwen`。
基线：`c24a325eafad7ff9783f26815605a3f1c9a97fc5`；以下测试发生于本单未提交工作树，不能标成基线提交已有能力。尚未合并main。
用户授权模型接入、项目局部下载及当前设计原件修订；Issue、独立评审与正式审批缺项保持Blocked。TASK-004未执行。

## 本单实现

- 项目已实际包含Qwen3.5-4B Q4_K_M GGUF（2,740,937,888字节）、llama.cpp b10964的Windows CPU/Vulkan运行库。原模型为Qwen，GGUF由Unsloth转换；版本与散列固定于 `config/local-ai.lock.json`。下载时核验官方发行散列，保留Apache-2.0、MIT与LLVM OpenMP许可。
- UE异步启动一个隐藏的本机子进程，使用动态127.0.0.1端口及临时认证，4096上下文、单并发、关闭thinking；玩家运行无需Python、云端API或另装模型管理器。进程随世界结束释放，Windows Job约束父进程生命周期。
- 规则意图候选可返回unknown，词项评分检索最多三个初始知识块；只传角色允许获知的世界状态。当前未部署Embedding模型、持久认知或第二个生成式LLM。
- 4B生成受互斥JSON Schema约束的collect或非动作结果，C++再次校验字段、类型、数量和三步计划；实际动作仍通过012执行器的30米、真实资源、安全、代次和epoch校验。暂停期间到达的回复等恢复后复核；取消回复不撤销已批准目标。
- Blueprint调用入口与非Shipping控制台入口已接通，HUD显示等待、回复与实际委托进度。模型结果不能直接编辑库存。
- Build.cs登记模型、两个后端的server/DLL、知识及许可证为NonUFS运行依赖；Git忽略权重/二进制，源码克隆需执行局部准备脚本。下载失败保留.part，续传完成后才能成为正式模型文件。

## 设计同步

用户批准的 [DSGN-001](../design/DSGN-001-local-inference.md) 已写入当前DOCX第11章：轻量意图处理 → RAG与可知状态过滤 → 一次4B本机推理 → 结构化候选和NPC台词 → UE校验执行。
当前原件通过本机Word编辑、导出PDF，检查改动页1/32/63；正文语义diff仅涉及封面日期、架构表/段落和附录技术边界。归档未改，DOCX的LFS锁保留至集成交接。
原TASK-013存档/认知快照规划保留为TASK-016，TASK-020依赖同步；没有执行存档任务。R14/R18/R20等未定玩法保持OPEN。

## 验证与发现

- Development Editor与Development Game / Win64均构建通过。Game构建收据列出68项本地AI运行依赖，全部文件存在且为NonUFS，含GGUF、两个server和两份OpenMP许可；见 [Game构建](../qa/evidence/TASK-013/build-game.json) 与 [运行依赖核对](../qa/evidence/TASK-013/staging-receipt.json)。
- 最终Vulkan模型版本为prompt task013-4、knowledge hearthward-initial-knowledge-4。真实模型10类输入、两轮PIE共43项自动检查通过；明确委托实际采集并入库10木材，资源从16减到6。
- 本机RTX4060 Laptop 8GB、32层Vulkan实测首次请求含加载8.93秒，后续本轮请求约2.0–3.0秒；不是跨机器性能承诺。默认CPU另做独立验证，不把GPU结果冒充CPU表现。
- 明确/模糊/危险/多目标/矛盾输入、虚报不改世界、暂停延迟执行、取消/覆盖、旧epoch、离开30米及新PIE隔离均有真实结果。
- 两张1280×720原生截图已目视检查，文本可读，模型回复区与实际进度区不重叠；不等同于物理DPI兼容性验证。
- CPU最终2类真实输入、9项检查通过：模型缺失明确失败且不执行；加载后能澄清；推理子进程意外退出不产生动作；紧接着重试能清理旧进程并重新加载。两次含加载响应为32.00秒、27.53秒。GPU和CPU本次测试的三个模型PID在结束后均已从OS进程表消失。
- 仓库工具自测31/31通过，包含本单运行库提取/扩展名缺失许可证回归。原生LocalAI 2/2、伙伴回归2/2通过，未重跑无关游戏模块。伙伴回归所覆盖的执行器代码随后未变；LocalAI在最后修复后重新验证。

初轮CPU明确采集成功，但含糊请求被误分类为dialogue。Vulkan初轮遇到非流式请求失败，发现UE默认30秒空闲上限与120秒总超时不一致，已对该请求统一为120秒。随后矛盾要求的clarify仍带wood/10，C++正确拒绝；已将Schema改为互斥分支，从生成阶段排除此类跨字段组合，未削弱世界校验。

结构与动作测试通过不代表台词全面合格。最终样本能识别真实库存10，未采用玩家虚报100；仍有不必要的出发前提问，以及已经在营地却补一句“等回营确认”的措辞问题。记录为4B台词质量限制，不能宣称角色认知已全面可靠。当前没有长期对话历史，不会真的保存台词中“暂且记着”的承诺。

最终CPU故障注入复现：HTTP失败先于进程退出Tick，立即重试曾复用旧进程。已在传输失败时清理自有进程，并在StartServer遇到已退出句柄时重新创建。此最后修复只影响失败恢复路径；Vulkan通过的成功推理、Schema、提示、执行与UI路径随后未变，CPU恢复路径另有最终复测。

关键证据：[Vulkan真实模型](../qa/evidence/TASK-013/real-model-vulkan.json)、[CPU与恢复](../qa/evidence/TASK-013/real-model-cpu.json)、[模型来源](../qa/evidence/TASK-013/bundle-provenance.json)、[设计检查](../qa/evidence/TASK-013/design-review.json)、[最终Editor构建](../qa/evidence/TASK-013/build-final.json)。早期失败和被拒绝的原始结构另行保留，未用最终PASS覆盖。

正式基线范围检查按旧TASK-013存档任务的范围执行，因此报告33个OUT_OF_SCOPE；用户本轮已将013改为本地模型并要求同步设计，当前任务单据此重写，旧存档范围保留在016。未修改检查器、未把旧基线检查声称为通过；按本轮任务单另做路径核对。Issue/评审门槛仍待正常集成流程补齐。

## 当前边界

模型接在012隔离开发夹具上：有限16木材、每趟重量4、五秒采集、直线碰撞移动与50cm到达距离仍为PROTOTYPE_ONLY。正式弟弟实体、全地图导航/危险识别、完整对话输入界面和长期记忆未交付。
正式输入界面归TASK-015；存档/知识快照归TASK-016。全局三条快捷建议仍按原设计主动刷新，本单未实现该界面。
完整游戏打包启动、两机复现和完整M0验收NOT_RUN；运行依赖登记不能代替这些验收。现有引擎启动期Condition failed诊断保留，不宣称日志无错误。

复现入口见 [BUILD_AND_TEST](../qa/BUILD_AND_TEST.md)。根README、设计入口、项目状态与Agent约定同步本单口径。
用户已明确授权本单代码、设计修订和证据提交推送。模型和下载运行库按约定留在被忽略的项目目录，不加入源码提交；提交绑定见下文。
参考：[llama.cpp server](https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md)、[JSON Schema约束](https://github.com/ggml-org/llama.cpp/blob/master/grammars/README.md)、[Qwen原模型](https://huggingface.co/Qwen/Qwen3.5-4B)、[Unsloth GGUF](https://huggingface.co/unsloth/Qwen3.5-4B-GGUF)。
