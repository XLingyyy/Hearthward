#!/usr/bin/env python3
"""Generate and immediately download Tripo 3D character models.

The API key is read only from TRIPO_API_KEY. It is never accepted on the
command line, printed, or written to a task record.
"""

from __future__ import annotations

import argparse
import asyncio
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Sequence


DEFAULT_MODEL = "v3.1-20260211"
DEFAULT_OUTPUT_DIR = Path(__file__).resolve().parent / "outputs"
SUCCESS_STATUSES = {"success", "succeeded", "completed", "complete"}


def _existing_file(value: str) -> Path:
    path = Path(value).expanduser().resolve()
    if not path.is_file():
        raise argparse.ArgumentTypeError(f"文件不存在: {path}")
    return path


def _face_limit(value: str) -> int:
    parsed = int(value)
    if not 500 <= parsed <= 1_500_000:
        raise argparse.ArgumentTypeError("--face-limit 必须在 500 到 1500000 之间")
    return parsed


def _positive_float(value: str) -> float:
    parsed = float(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("数值必须大于 0")
    return parsed


def _add_generation_options(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--model", default=DEFAULT_MODEL, help=f"Tripo 模型版本（默认 {DEFAULT_MODEL}）")
    parser.add_argument("--face-limit", type=_face_limit, default=80_000, help="最大面数（默认 80000）")
    parser.add_argument("--no-texture", action="store_true", help="不生成纹理")
    parser.add_argument("--no-pbr", action="store_true", help="不生成 PBR 材质")
    parser.add_argument(
        "--texture-quality",
        choices=("standard", "detailed"),
        default="detailed",
        help="纹理质量（默认 detailed）",
    )
    parser.add_argument("--model-seed", type=int, help="固定几何随机种子")
    parser.add_argument("--texture-seed", type=int, help="固定纹理随机种子")
    parser.add_argument("--quad", action="store_true", help="请求四边面；Tripo 可能输出 FBX")
    parser.add_argument("--smart-low-poly", action="store_true", help="启用智能低模拓扑")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="调用 Tripo API 生成主角 3D 模型，并把临时下载地址中的文件立即保存到本地。"
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help=f"输出根目录（默认 {DEFAULT_OUTPUT_DIR}）",
    )
    parser.add_argument("--poll-interval", type=_positive_float, default=2.0, help="轮询间隔秒数（默认 2）")
    parser.add_argument("--timeout", type=_positive_float, default=900.0, help="等待超时秒数（默认 900）")
    parser.add_argument("--no-wait", action="store_true", help="只提交并保存 task_id，不等待和下载")
    parser.add_argument("--dry-run", action="store_true", help="只校验并打印请求摘要，不联网、不消耗额度")

    subparsers = parser.add_subparsers(dest="command", required=True)

    text_parser = subparsers.add_parser("text", help="文本生成 3D")
    prompt_group = text_parser.add_mutually_exclusive_group(required=True)
    prompt_group.add_argument("--prompt", help="生成提示词")
    prompt_group.add_argument("--prompt-file", type=_existing_file, help="UTF-8 提示词文件")
    text_parser.add_argument("--negative-prompt", help="反向提示词，最长 255 字符")
    text_parser.add_argument("--image-seed", type=int, help="固定文本阶段随机种子")
    _add_generation_options(text_parser)

    image_parser = subparsers.add_parser("image", help="单张参考图生成 3D")
    image_parser.add_argument("image", type=_existing_file, help="主角参考图（PNG/JPG，建议完整无遮挡全身图）")
    image_parser.add_argument(
        "--texture-alignment",
        choices=("original_image", "geometry"),
        default="original_image",
        help="纹理优先贴合原图或几何",
    )
    image_parser.add_argument(
        "--orientation",
        choices=("default", "align_image"),
        default="align_image",
        help="是否按原图自动校正朝向",
    )
    _add_generation_options(image_parser)

    multi_parser = subparsers.add_parser("multiview", help="四视图生成 3D")
    multi_parser.add_argument("--front", type=_existing_file, required=True, help="正面图")
    multi_parser.add_argument("--left", type=_existing_file, required=True, help="角色左侧图")
    multi_parser.add_argument("--back", type=_existing_file, required=True, help="背面图")
    multi_parser.add_argument("--right", type=_existing_file, required=True, help="角色右侧图")
    multi_parser.add_argument(
        "--texture-alignment",
        choices=("original_image", "geometry"),
        default="original_image",
    )
    multi_parser.add_argument(
        "--orientation", choices=("default", "align_image"), default="align_image"
    )
    _add_generation_options(multi_parser)

    status_parser = subparsers.add_parser("status", help="继续查询已有任务，并在成功后下载")
    status_parser.add_argument("task_id", help="Tripo task_id")

    subparsers.add_parser("balance", help="查询账户余额")
    return parser


def _read_prompt(args: argparse.Namespace) -> str:
    prompt = args.prompt if args.prompt is not None else args.prompt_file.read_text(encoding="utf-8")
    prompt = prompt.strip()
    if not prompt:
        raise ValueError("提示词不能为空")
    if len(prompt) > 1024:
        raise ValueError(f"提示词长度为 {len(prompt)}，超过 Tripo 的 1024 字符限制")
    if args.negative_prompt and len(args.negative_prompt) > 255:
        raise ValueError("反向提示词超过 Tripo 的 255 字符限制")
    return prompt


def _common_options(args: argparse.Namespace) -> dict[str, Any]:
    options: dict[str, Any] = {
        "model_version": args.model,
        "face_limit": args.face_limit,
        "texture": not args.no_texture,
        "pbr": not args.no_pbr and not args.no_texture,
        "texture_quality": args.texture_quality,
        "quad": args.quad,
        "smart_low_poly": args.smart_low_poly,
    }
    if args.model_seed is not None:
        options["model_seed"] = args.model_seed
    if args.texture_seed is not None:
        options["texture_seed"] = args.texture_seed
    return options


def build_request_summary(args: argparse.Namespace) -> dict[str, Any]:
    if args.command == "status":
        return {"command": "status", "task_id": args.task_id}
    if args.command == "balance":
        return {"command": "balance"}

    options = _common_options(args)
    model = options["model_version"]
    face_limit = options["face_limit"]
    if model == "P1-20260311":
        if not 500 <= face_limit <= 20_000:
            raise ValueError("P1-20260311 的 --face-limit 必须在 500 到 20000 之间")
        if options["quad"] or options["smart_low_poly"]:
            raise ValueError("P1-20260311 不接受 --quad 或 --smart-low-poly")
        # P1 only accepts the smaller parameter set documented by Tripo.
        options.pop("texture_quality")
    elif options["quad"] and face_limit > 150_000:
        raise ValueError("启用 --quad 时 --face-limit 不能超过 150000")
    if options["smart_low_poly"]:
        maximum = 10_000 if options["quad"] else 20_000
        if not 1_000 <= face_limit <= maximum:
            raise ValueError(
                f"启用 --smart-low-poly 时 --face-limit 必须在 1000 到 {maximum} 之间"
            )
    if args.command == "text":
        options.update({"command": "text", "prompt": _read_prompt(args)})
        if args.negative_prompt:
            options["negative_prompt"] = args.negative_prompt
        if args.image_seed is not None:
            options["image_seed"] = args.image_seed
    elif args.command == "image":
        options.update(
            {
                "command": "image",
                "image": str(args.image),
                "texture_alignment": args.texture_alignment,
                "orientation": args.orientation,
            }
        )
    elif args.command == "multiview":
        options.update(
            {
                "command": "multiview",
                "images": [str(args.front), str(args.left), str(args.back), str(args.right)],
                "image_order": ["front", "left", "back", "right"],
                "texture_alignment": args.texture_alignment,
                "orientation": args.orientation,
            }
        )
    return options


def _sdk_imports() -> tuple[Any, Any]:
    try:
        from tripo3d import TripoClient
        from tripo3d.models import TaskStatus
    except ImportError as exc:
        raise RuntimeError(
            "缺少 Tripo SDK。请先运行: python -m pip install -r requirements.txt"
        ) from exc
    return TripoClient, TaskStatus


def _require_api_key() -> str:
    api_key = os.environ.get("TRIPO_API_KEY", "").strip()
    if not api_key:
        raise RuntimeError("未设置 TRIPO_API_KEY 环境变量")
    return api_key


def _task_status_text(task: Any) -> str:
    status = getattr(task, "status", "")
    value = getattr(status, "value", status)
    return str(value).lower()


def _jsonable(value: Any) -> Any:
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, Path):
        return str(value)
    if isinstance(value, dict):
        return {str(key): _jsonable(item) for key, item in value.items()}
    if isinstance(value, (list, tuple, set)):
        return [_jsonable(item) for item in value]
    enum_value = getattr(value, "value", None)
    if enum_value is not None:
        return _jsonable(enum_value)
    if hasattr(value, "model_dump"):
        return _jsonable(value.model_dump())
    if hasattr(value, "dict"):
        return _jsonable(value.dict())
    if hasattr(value, "__dict__"):
        return {
            key: _jsonable(item)
            for key, item in vars(value).items()
            if not key.startswith("_")
        }
    return str(value)


def _write_record(folder: Path, record: dict[str, Any]) -> Path:
    folder.mkdir(parents=True, exist_ok=True)
    path = folder / "task.json"
    path.write_text(json.dumps(_jsonable(record), ensure_ascii=False, indent=2), encoding="utf-8")
    return path


def _load_record(folder: Path) -> dict[str, Any] | None:
    path = folder / "task.json"
    if not path.is_file():
        return None
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None
    return value if isinstance(value, dict) else None


async def _submit(client: Any, args: argparse.Namespace, summary: dict[str, Any]) -> str:
    common = {
        key: value
        for key, value in summary.items()
        if key
        in {
            "model_version",
            "face_limit",
            "texture",
            "pbr",
            "texture_quality",
            "model_seed",
            "texture_seed",
            "quad",
            "smart_low_poly",
        }
    }
    if args.command == "text":
        return await client.text_to_model(
            prompt=summary["prompt"],
            negative_prompt=summary.get("negative_prompt"),
            image_seed=summary.get("image_seed"),
            **common,
        )
    image_options = {
        **common,
        "texture_alignment": summary["texture_alignment"],
        "orientation": summary["orientation"],
    }
    if args.command == "image":
        return await client.image_to_model(image=summary["image"], **image_options)
    return await client.multiview_to_model(images=summary["images"], **image_options)


async def _download_successful_task(client: Any, task: Any, folder: Path) -> dict[str, str]:
    status = _task_status_text(task)
    if status not in SUCCESS_STATUSES:
        raise RuntimeError(f"Tripo 任务未成功，终态为: {status}")
    folder.mkdir(parents=True, exist_ok=True)
    downloaded = await client.download_task_models(task, str(folder))
    result = {key: value for key, value in downloaded.items() if value}
    if not result:
        raise RuntimeError("任务成功，但响应中没有可下载的模型或预览图")
    return result


async def run(args: argparse.Namespace) -> int:
    summary = build_request_summary(args)
    if args.dry_run:
        print(json.dumps(summary, ensure_ascii=False, indent=2))
        print("预演完成：未连接 Tripo，未消耗额度。")
        return 0

    api_key = _require_api_key()
    TripoClient, _ = _sdk_imports()
    output_root = args.output_dir.expanduser().resolve()

    async with TripoClient(api_key=api_key) as client:
        if args.command == "balance":
            balance = await client.get_balance()
            print(json.dumps(_jsonable(balance), ensure_ascii=False, indent=2))
            return 0

        if args.command == "status":
            task_id = args.task_id
        else:
            task_id = await _submit(client, args, summary)
            print(f"任务已提交: {task_id}")

        task_folder = output_root / task_id
        existing_record = _load_record(task_folder) if args.command == "status" else None
        if existing_record:
            base_record = {
                **existing_record,
                "resumed_at_utc": datetime.now(timezone.utc).isoformat(),
                "resume_request": summary,
            }
        else:
            base_record = {
                "task_id": task_id,
                "created_at_utc": datetime.now(timezone.utc).isoformat(),
                "request": summary,
            }
        _write_record(task_folder, base_record)

        if args.no_wait:
            print(f"任务记录: {task_folder / 'task.json'}")
            return 0

        print("等待 Tripo 完成任务……")
        task = await client.wait_for_task(
            task_id,
            polling_interval=args.poll_interval,
            timeout=args.timeout,
            verbose=True,
        )
        status = _task_status_text(task)
        final_record = {**base_record, "status": status, "response": _jsonable(task)}
        _write_record(task_folder, final_record)
        files = await _download_successful_task(client, task, task_folder)
        final_record["downloaded_files"] = files
        record_path = _write_record(task_folder, final_record)

        print(f"任务成功: {task_id}")
        for kind, path in files.items():
            print(f"  {kind}: {path}")
        print(f"任务记录: {record_path}")
        return 0


def main(argv: Sequence[str] | None = None) -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return asyncio.run(run(args))
    except KeyboardInterrupt:
        print("已取消。任务若已提交，可用 status 子命令继续查询。", file=sys.stderr)
        return 130
    except Exception as exc:
        print(f"错误: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
