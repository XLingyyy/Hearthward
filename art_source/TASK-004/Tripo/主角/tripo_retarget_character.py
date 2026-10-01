#!/usr/bin/env python3
"""Create and download a basic Unreal locomotion/combat animation set."""

from __future__ import annotations

import argparse
import asyncio
import json
import os
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from urllib.parse import urlparse

from tripo_generate import _jsonable, _task_status_text, _write_record
from tripo_rig_character import SUCCESS_STATUSES, wait_with_reconnect


ANIMATION_NAMES = ("idle", "walk", "run", "slash", "hurt")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="为 Tripo 双足骨架生成 UE 移动/战斗动画。")
    parser.add_argument("rig_task_id", help="成功的 Tripo 绑定 task_id")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "outputs",
    )
    parser.add_argument("--timeout", type=float, default=1200.0)
    parser.add_argument("--poll-interval", type=float, default=2.0)
    return parser


def collect_urls(value: Any, path: tuple[str, ...] = ()) -> list[tuple[tuple[str, ...], str]]:
    found: list[tuple[tuple[str, ...], str]] = []
    if isinstance(value, str) and value.startswith(("https://", "http://")):
        found.append((path, value))
    elif isinstance(value, dict):
        for key, item in value.items():
            found.extend(collect_urls(item, (*path, str(key))))
    elif isinstance(value, list):
        for index, item in enumerate(value):
            found.extend(collect_urls(item, (*path, str(index))))
    return found


def safe_filename(path: tuple[str, ...], url: str, index: int) -> str:
    url_path = urlparse(url).path
    extension = Path(url_path).suffix.lower()
    if not extension or len(extension) > 8:
        extension = ".fbx"
    if index < len(ANIMATION_NAMES):
        base = ANIMATION_NAMES[index]
    else:
        base = "_".join(path[-2:]) if path else f"animation_{index + 1}"
    base = re.sub(r"[^A-Za-z0-9_-]+", "_", base).strip("_") or f"animation_{index + 1}"
    return f"{index + 1:02d}_{base}{extension}"


async def run(args: argparse.Namespace) -> int:
    api_key = os.environ.get("TRIPO_API_KEY", "").strip()
    if not api_key:
        raise RuntimeError("未设置 TRIPO_API_KEY")

    from tripo3d import TripoClient
    from tripo3d.models import Animation

    output_root = args.output_dir.resolve()
    source_folder = output_root / args.rig_task_id
    pipeline_path = source_folder / "animation_pipeline.json"
    state: dict[str, Any] = {
        "rig_task_id": args.rig_task_id,
        "started_at_utc": datetime.now(timezone.utc).isoformat(),
        "animations": list(ANIMATION_NAMES),
        "out_format": "fbx",
        "animate_in_place": True,
        "export_with_geometry": False,
    }
    pipeline_path.parent.mkdir(parents=True, exist_ok=True)
    pipeline_path.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")

    presets = [Animation.IDLE, Animation.WALK, Animation.RUN, Animation.SLASH, Animation.HURT]
    async with TripoClient(api_key=api_key) as client:
        task_id = await client.retarget_animation(
            original_model_task_id=args.rig_task_id,
            animation=presets,
            out_format="fbx",
            bake_animation=True,
            export_with_geometry=False,
            animate_in_place=True,
        )
        state["animation_task_id"] = task_id
        pipeline_path.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"动画任务已提交: {task_id}")

        task = await wait_with_reconnect(
            client,
            task_id,
            polling_interval=args.poll_interval,
            timeout=args.timeout,
        )
        status = _task_status_text(task)
        if status not in SUCCESS_STATUSES:
            raise RuntimeError(f"动画任务失败: {status}")

        raw = await client._impl._request("GET", f"/task/{task_id}")
        raw_data = raw.get("data", {})
        output = raw_data.get("output", {})
        urls = collect_urls(output)
        if not urls:
            raise RuntimeError("动画任务成功，但响应中没有可下载文件")

        task_folder = output_root / task_id
        task_folder.mkdir(parents=True, exist_ok=True)
        downloaded: dict[str, str] = {}
        for index, (url_path, url) in enumerate(urls):
            filename = safe_filename(url_path, url, index)
            destination = task_folder / filename
            await client._download_with_ssl_retry(url, str(destination))
            downloaded["/".join(url_path) or str(index)] = str(destination)
            print(f"  已下载: {destination}")

        state.update(
            {
                "status": status,
                "completed_at_utc": datetime.now(timezone.utc).isoformat(),
                "downloaded_files": downloaded,
            }
        )
        pipeline_path.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
        _write_record(
            task_folder,
            {
                "task_id": task_id,
                "status": status,
                "source_rig_task_id": args.rig_task_id,
                "animations": list(ANIMATION_NAMES),
                "animate_in_place": True,
                "response": raw_data,
                "downloaded_files": downloaded,
            },
        )
        print(f"动画生成成功: {task_id}")
        print(f"流水线记录: {pipeline_path}")
    return 0


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    args = build_parser().parse_args()
    try:
        return asyncio.run(run(args))
    except KeyboardInterrupt:
        print("已取消。本地停止不会取消已提交的 Tripo 云端任务。", file=sys.stderr)
        return 130
    except Exception as exc:
        print(f"错误: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
