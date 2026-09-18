# Hearthward 本地推理包

准备：在仓库根目录执行 `python scripts/local_ai/prepare_bundle.py`。脚本只写此项目，下载固定版本、核验发行物散列；意外中断后保留.part，重新运行可续传。
运行不需要Python；UE直接启动bin/cpu或bin/vulkan中的llama-server.exe，加载models/Qwen3.5-4B-Q4_K_M.gguf。
Config/DefaultGame.ini的Hearthward.LocalAI段选择Backend=cpu或vulkan和GpuLayers。默认CPU；GPU模式需兼容驱动和足够余量，具体性能见013证据。开发启动可用 `-HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=32` 覆盖，CPU始终使用0层GPU卸载。

版本来源见config/local-ai.lock.json。原模型Qwen/Qwen3.5-4B（Apache-2.0），GGUF转换提供者unsloth/Qwen3.5-4B-GGUF；运行库ggml-org/llama.cpp b10964（MIT）。保留本目录许可文件，以及两个后端目录中的LICENSE-LLVM-OpenMP，发行时随资源一起分发。
权重与下载的二进制在项目目录实际存在，Git忽略这些大文件；源码克隆需要准备步骤，玩家发行包由Unreal RuntimeDependencies以NonUFS收集所需文件。
开发时不得把工作流/AGENTS/GDD全集直接作为NPC知识包。knowledge.json只含可用于角色的初始规则，动态世界由UE过滤后提供。
