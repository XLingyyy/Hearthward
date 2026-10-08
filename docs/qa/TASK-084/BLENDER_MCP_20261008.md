# Blender MCP 本地连接记录

2026-10-08，Owner 明确授权连接本地 Blender MCP 并操作制作资源。

- Blender：`E:/Tools/Blender-5.2.0-LTS/blender.exe`，5.2.0 LTS portable。
- 上游：[ahujasid/mcp-for-blender](https://github.com/ahujasid/mcp-for-blender)，MIT，固定提交 `7a0373ec9199183cb460068c4f96aed9c579fb4f`，服务包版本 2.1.9。
- 上游源码：`.agent-local/tools/blender-mcp`；独立 Python 环境：`E:/Tools/Hearthward-BlenderMCP/venv`。未修改全局 Python 或 GameFactory 主环境。初次环境置于项目内，仓库链接检查会扫描第三方包文档，因此已迁到工具目录并重新验证连接。
- 已通过 Blender API 安装和启用插件：`E:/Tools/Blender-5.2.0-LTS/portable/scripts/addons/blender_mcp.py`，并保存 portable 用户偏好。Blender 插件监听 `127.0.0.1:9876`。
- GameFactory 与 Hearthward 两处 `.codex/config.toml` 均配置 `mcp_servers.blender`；stdio 命令指向上述独立环境的 `Scripts/mcp-for-blender.exe`。配置依据 [Codex MCP 文档](https://developers.openai.com/codex/mcp/)。这些路径为本机配置，其他机器需要调整。
- `BLENDER_MCP_DISABLE_TELEMETRY=1`，插件 `telemetry_consent=False`；未调用云生成、付费服务或外部资产接口。

实际完成 MCP initialize、list_tools、get_scene_info 与 execute_blender_code；通过该连接完成场景检查、建模、UV、颜色烘焙、FBX 导出和渲染，成果见 [弓箭装备报告](../TASK-095/archery-kit/REPORT.md)。使用客户端为 `.agent-local/tools/blender_mcp_call.py`（官方 Python MCP ClientSession + stdio），不是直接向插件端口伪造测试响应。

当前聊天的原生工具目录不支持本轮热挂载，实际操作走同一 MCP 服务的 Python 客户端。新聊天加载配置后的原生工具挂载尚未验证，不宣称已验证。Blender 必须保持运行且插件服务开启；关闭 Blender 后这条连接不可用。

安装来源与操作记录保存在 `.agent-local/tools/`。原 `Archer.blend` 切除试验没有保存；本轮新增可编辑资源为 `art_source/TASK-095/ArcheryKit.blend`。
