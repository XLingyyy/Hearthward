#!/usr/bin/env python3
"""Rig a successful Tripo model task as a Mixamo-compatible biped FBX."""

from __future__ import annotations

import argparse
import asyncio
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from tripo_generate import _download_successful_task, _jsonable, _task_status_text, _write_record


SUCCESS_STATUSES = {"success", "succeeded", "completed", "complete"}


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="检查并绑定 Tripo 双足人物骨架，输出 Mixamo FBX。")
    parser.add_argument("original_task_id", help="成功的 Tripo 建模 task_id")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "outputs",
        help="输出根目录",
    )
    parser.add_argument("--timeout", type=float, default=1200.0, help="每个阶段的超时秒数")
    parser.add_argument("--poll-interval", type=float, default=2.0, help="初始轮询间隔秒数")
    return parser


def write_pipeline(path: Path, state: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(_jsonable(state), ensure_ascii=False, indent=2), encoding="utf-8")


async def wait_with_reconnect(
    client: Any,
    task_id: str,
    *,
    polling_interval: float,
    timeout: float,
    attempts: int = 8,
) -> Any:
    for attempt in range(1, attempts + 1):
        try:
            return await client.wait_for_task(
                task_id,
                polling_interval=polling_interval,
                timeout=timeout,
                verbose=True,
            )
        except asyncio.TimeoutError:
            raise
        except Exception as exc:
            if attempt == attempts:
                raise
            delay = min(3 * attempt, 20)
            print(f"连接中断（{exc}），{delay} 秒后继续查询同一任务……", file=sys.stderr)
            await asyncio.sleep(delay)
    raise RuntimeError("无法取得 Tripo 任务状态")


async def run(args: argparse.Namespace) -> int:
    api_key = os.environ.get("TRIPO_API_KEY", "").strip()
    if not api_key:
        raise RuntimeError("未设置 TRIPO_API_KEY")

    from tripo3d import TripoClient
    from tripo3d.models import RigSpec, RigType

    original_folder = args.output_dir.resolve() / args.original_task_id
    pipeline_path = original_folder / "rig_pipeline.json"
    state: dict[str, Any] = {
        "original_task_id": args.original_task_id,
        "started_at_utc": datetime.now(timezone.utc).isoformat(),
        "settings": {
            "rig_type": "biped",
            "spec": "mixamo",
            "out_format": "fbx",
            "model_version": "v2.0-20250506",
        },
    }
    write_pipeline(pipeline_path, state)

    async with TripoClient(api_key=api_key) as client:
        check_id = await client.check_riggable(args.original_task_id)
        state["rig_check_task_id"] = check_id
        write_pipeline(pipeline_path, state)
        print(f"骨骼兼容性检查已提交: {check_id}")

        check_task = await wait_with_reconnect(
            client,
            check_id,
            polling_interval=args.poll_interval,
            timeout=args.timeout,
        )
        check_status = _task_status_text(check_task)
        state["rig_check"] = _jsonable(check_task)
        write_pipeline(pipeline_path, state)
        if check_status not in SUCCESS_STATUSES:
            raise RuntimeError(f"骨骼兼容性检查失败: {check_status}")

        riggable = bool(getattr(check_task.output, "riggable", False))
        recommended = getattr(check_task.output, "rig_type", None)
        recommended = getattr(recommended, "value", recommended)
        print(f"检查结果: riggable={riggable}, rig_type={recommended}")
        if not riggable:
            raise RuntimeError("Tripo 判定该模型不能自动绑定骨架")
        if recommended not in (None, "", "biped"):
            raise RuntimeError(f"Tripo 建议的骨骼类型不是 biped，而是: {recommended}")

        rig_id = await client.rig_model(
            original_model_task_id=args.original_task_id,
            model_version="v2.0-20250506",
            out_format="fbx",
            rig_type=RigType.BIPED,
            spec=RigSpec.MIXAMO,
        )
        state["rig_task_id"] = rig_id
        write_pipeline(pipeline_path, state)
        print(f"自动绑定任务已提交: {rig_id}")

        rig_task = await wait_with_reconnect(
            client,
            rig_id,
            polling_interval=args.poll_interval,
            timeout=args.timeout,
        )
        rig_status = _task_status_text(rig_task)
        state["rig_status"] = rig_status
        state["rig_response"] = _jsonable(rig_task)
        write_pipeline(pipeline_path, state)
        if rig_status not in SUCCESS_STATUSES:
            raise RuntimeError(f"自动绑定任务失败: {rig_status}")

        rig_folder = args.output_dir.resolve() / rig_id
        files = await _download_successful_task(client, rig_task, rig_folder)
        state["completed_at_utc"] = datetime.now(timezone.utc).isoformat()
        state["downloaded_files"] = files
        write_pipeline(pipeline_path, state)
        _write_record(
            rig_folder,
            {
                "task_id": rig_id,
                "status": rig_status,
                "source_task_id": args.original_task_id,
                "settings": state["settings"],
                "response": _jsonable(rig_task),
                "downloaded_files": files,
            },
        )

        print(f"骨骼绑定成功: {rig_id}")
        for kind, path in files.items():
            print(f"  {kind}: {path}")
        print(f"流水线记录: {pipeline_path}")
    return 0


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    parser = build_parser()
    args = parser.parse_args()
    try:
        return asyncio.run(run(args))
    except KeyboardInterrupt:
        print("已取消。本地停止不会取消已经提交的 Tripo 云端任务。", file=sys.stderr)
        return 130
    except Exception as exc:
        print(f"错误: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
