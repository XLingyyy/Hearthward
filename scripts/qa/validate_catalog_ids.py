#!/usr/bin/env python3
"""Read-only ID integrity checks for tables used by the catalog lookup fast path.

Uses the standard library only. This is NOT a UE build, gameplay test, performance
measurement or complete catalog schema validator. It never repairs source data.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import sys
from typing import Any

TABLE_PATHS = (
    ("items",), ("skills",), ("craftingRecipes",), ("repairRecipes",),
    ("locations",), ("quests",), ("encounters",),
    ("campaign", "quests"), ("campaign", "locations"),
    ("campaign", "zones"), ("campaign", "enemies"), ("campaign", "people"),
)


def unique_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"Duplicate JSON object key: {key}")
        result[key] = value
    return result


def finite_float(text: str) -> float:
    value = float(text)
    if not math.isfinite(value):
        raise ValueError("Non-finite JSON number")
    return value


def reject_constant(text: str) -> Any:
    raise ValueError(f"Non-standard JSON numeric constant: {text}")


def load_catalog(path: Path) -> Any:
    # utf-8-sig accepts the UTF-8 BOM without silently accepting legacy encodings.
    return json.loads(path.read_text(encoding="utf-8-sig"),
                      object_pairs_hook=unique_object,
                      parse_float=finite_float, parse_constant=reject_constant)


def validate_catalog(root: Any) -> dict[str, Any]:
    errors: list[str] = []
    counts: dict[str, int] = {}
    if not isinstance(root, dict):
        return {"status": "FAIL", "errors": ["Catalog root must be an object"], "table_rows": {}}
    for path in TABLE_PATHS:
        label = ".".join(path)
        rows: Any = root
        for key in path:
            if not isinstance(rows, dict) or key not in rows:
                errors.append(f"{label}: missing table or invalid parent object")
                rows = None
                break
            rows = rows[key]
        if rows is None:
            # A null table is not accepted; avoid adding a redundant diagnostic.
            if not any(e.startswith(label + ":") for e in errors):
                errors.append(f"{label}: table must be an array, not null")
            continue
        if not isinstance(rows, list):
            errors.append(f"{label}: table must be an array")
            continue
        counts[label] = len(rows)
        seen: dict[str, int] = {}
        for index, row in enumerate(rows):
            if not isinstance(row, dict):
                errors.append(f"{label}[{index}]: row must be an object")
                continue
            row_id = row.get("id")
            if not isinstance(row_id, str) or not row_id.strip():
                errors.append(f"{label}[{index}]: id must be a nonempty string")
                continue
            if row_id in seen:
                errors.append(f"{label}[{index}]: duplicate id {row_id!r}; first row {seen[row_id]}")
            else:
                seen[row_id] = index
    return {"status": "FAIL" if errors else "PASS", "errors": errors, "table_rows": counts}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path,
                        default=Path(__file__).resolve().parents[2] / "Resources/Data/gameplay.json")
    parser.add_argument("--json", action="store_true", help="Print machine-readable output; do not write a file")
    args = parser.parse_args(argv)
    try:
        report = validate_catalog(load_catalog(args.catalog))
    except (OSError, UnicodeError, ValueError, RecursionError) as exc:
        report = {"status": "FAIL", "errors": [f"Cannot read catalog: {type(exc).__name__}: {exc}"], "table_rows": {}}
    if args.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
    else:
        print(f"Catalog IDs: {report['status']} ({len(report['table_rows'])} tables checked)")
        for error in report["errors"]:
            print(error)
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
