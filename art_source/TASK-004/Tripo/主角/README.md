# Tripo 主角建模工具

这个目录提供一个独立的 Python 命令行工具，用 Tripo API 创建文生 3D、单图生 3D 或四视图生 3D 任务，并在任务完成后立即下载模型、PBR 模型与渲染预览图。

API key 只从进程环境变量 `TRIPO_API_KEY` 读取，不会写进代码、命令历史参数或 `task.json`。每个任务会保存在 `outputs/<task_id>/`，其中 `task.json` 记录请求摘要、最终状态与本地文件路径。

## 1. 安装

在 PowerShell 中进入本目录：

```powershell
cd 'E:\AiAgent\XLingGame\Resource\Tripo\主角'
py -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
```

如果 PowerShell 阻止激活脚本，也可以不激活，后续把 `python` 换成 `.\.venv\Scripts\python.exe`。

## 2. 先做无费用预演

示例提示词只依据当前项目已确认的“主角是首领和强大战士、需要可用于游戏的人形基模”等信息编写，不替项目决定年龄、族群或最终文化/美术风格。正式生成前请复制并修改提示词：

```powershell
Copy-Item '.\主角提示词.example.txt' '.\主角提示词.txt'
python .\tripo_generate.py --dry-run text --prompt-file '.\主角提示词.txt'
```

预演只打印请求摘要，不联网，也不消耗 Tripo 额度。

## 3. 设置 API key 并生成

在当前 PowerShell 会话中设置 key（关闭窗口后失效）：

```powershell
$env:TRIPO_API_KEY = 'tsk_你的密钥'
```

如果不想让 key 出现在聊天中，也可以新建本地文件 `.tripo_api_key`，只写一行 `tsk_...`。该文件已加入 `.gitignore`，专用启动脚本会自动读取且不会输出内容。

### 本次中世纪哥特主角

参考图已经复制到 `reference/主角-中世纪哥特-正面.png`，并配置为 v3.1、80,000 面、详细 PBR 纹理、自动朝向和四边面 FBX 输出。先预演：

```powershell
.\生成中世纪哥特主角.ps1 -DryRun
```

设置 API key 后正式提交并下载：

```powershell
.\生成中世纪哥特主角.ps1
```

文本生成：

```powershell
python .\tripo_generate.py text --prompt-file '.\主角提示词.txt'
```

单张参考图生成（更适合已有主角概念图的情况）：

```powershell
python .\tripo_generate.py image '.\reference\hero-front.png'
```

四视图生成，顺序固定为正、左、背、右：

```powershell
python .\tripo_generate.py multiview `
  --front '.\reference\front.png' `
  --left  '.\reference\left.png' `
  --back  '.\reference\back.png' `
  --right '.\reference\right.png'
```

查询余额：

```powershell
python .\tripo_generate.py balance
```

只提交、不等待：

```powershell
python .\tripo_generate.py --no-wait text --prompt-file '.\主角提示词.txt'
```

之后用输出的 `task_id` 继续等待并下载：

```powershell
python .\tripo_generate.py status '<task_id>'
```

## 常用参数

- `--face-limit 80000`：最大面数。主角质量与 UE 运行成本需要在导入后实测。
- `--model v3.1-20260211`：默认使用当前脚本验证的模型版本。
- `--texture-quality detailed`：详细纹理（默认）。
- `--model-seed N --texture-seed N`：固定随机种子，便于复现和做纹理变体。
- `--quad`：请求四边面；Tripo 可能因此输出 FBX。
- `--smart-low-poly`：请求低模拓扑；复杂角色可能失败。
- 使用 `P1-20260311` 时，面数必须为 500–20000，且不能同时使用 `--quad` 或 `--smart-low-poly`。
- `--output-dir <目录>`：更改本地输出根目录。
- `--timeout 900`：更改等待超时时间。超时不会取消云端任务，可再用 `status` 查询。

全局参数要放在子命令前，例如：

```powershell
python .\tripo_generate.py --timeout 1200 --output-dir '.\outputs' text --prompt-file '.\主角提示词.txt' --face-limit 120000
```

## 输入图建议

- PNG/JPG，分辨率至少 256×256；使用清晰、完整、无遮挡的全身图。
- 单人、纯净背景、四肢与身体分离，使用对称 A-pose 或 T-pose，避免手持武器和底座。
- 四视图必须是同一角色、同一服装、同一比例；方向按正面、角色左侧、背面、角色右侧提供。
- 生成模型仍需在 Blender/Maya 中检查拓扑、骨骼适配、UV、材质和比例，再导入 UE；Tripo 生成并不自动完成 UE 骨骼绑定或动画重定向。

## 测试

测试不连接 Tripo，不需要 API key：

```powershell
python -m unittest -v
```

官方资料：[SDK 集成](https://developers.tripo3d.ai/en/docs/sdk)、[Generation API](https://platform.tripo3d.ai/docs/generation)。
