"""Read original UE PNGs to check actual category highlights; never edit images."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent
CATEGORIES = ("main", "side", "world", "people", "factions", "collection")


def inspect(path, category):
    with Image.open(path) as source:
        image = source.convert("RGB")
        scale = min(image.width / 1672, image.height / 941)
        ox, oy = (image.width - 1672 * scale) / 2, (image.height - 941 * scale) / 2
        counts, peaks, bright_counts = [], [], []
        for index in range(6):
            bounds = tuple(round(value) for value in (
                ox + (560 + index * 100) * scale, oy + 28 * scale,
                ox + (624 + index * 100) * scale, oy + 95 * scale,
            ))
            pixels = image.crop(bounds).get_flattened_data()
            counts.append(sum(r > 115 and r - b > 25 and g - b > 15 for r, g, b in pixels))
            peaks.append(max((r + g + b) / 3 for r, g, b in pixels))
            bright_counts.append(sum(r >= 205 and g >= 185 and b >= 155 for r, g, b in pixels))
    active = [index for index, count in enumerate(counts) if count >= 10]
    chosen = CATEGORIES.index(category)
    brighter = peaks[chosen] > max(peak for index, peak in enumerate(peaks) if index != chosen) + 5
    return {
        "path": str(path.relative_to(ROOT)), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "category": category, "warm_pixel_counts": counts, "peak_brightness": peaks,
        "bright_pixel_counts": bright_counts,
        "warm_categories": [CATEGORIES[index] for index in active],
        "only_selected_warm": active == [chosen], "selected_brighter_than_inactive": brighter,
        "passed": active == [chosen] and brighter,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-run", required=True)
    parser.add_argument("--world-run", required=True)
    args = parser.parse_args()
    captures = []
    for category in CATEGORIES:
        native = ROOT / args.native_run / f"{args.native_run}-journal-{category}-known.png"
        world = ROOT / args.world_run / f"journal-{category}.png"
        captures.extend((inspect(native, category), inspect(world, category)))
    def compare_reference(capture, reference):
        chosen = CATEGORIES.index(capture["category"])
        capture["main_reference_peak"] = reference["peak_brightness"][0]
        capture["selected_matches_reference_level"] = (
            capture["peak_brightness"][chosen] >= max(215, capture["main_reference_peak"] * .9)
            and capture["bright_pixel_counts"][chosen] >= 20
        )
        capture["passed"] &= capture["selected_matches_reference_level"]

    for index, capture in enumerate(captures):
        compare_reference(capture, captures[index % 2])
    # Both historical defects must be detected by the same original-pixel audit.
    original = inspect(ROOT / "verify_1314147b9663/verify_1314147b9663-journal-side-known.png", "side")
    dim_reference = inspect(ROOT / "verify_0550668a7af1/verify_0550668a7af1-journal-main-known.png", "main")
    dim_original = inspect(ROOT / "verify_0550668a7af1/verify_0550668a7af1-journal-side-known.png", "side")
    compare_reference(dim_original, dim_reference)
    result = {
        "passed": all(capture["passed"] for capture in captures) and not original["passed"] and not dim_original["passed"],
        "captures": captures, "original_defect_detected": not original["passed"], "original": original,
        "dim_selection_defect_detected": not dim_original["passed"], "dim_original": dim_original,
        "method": "Original PNG header pixels: exactly one warm icon, brighter than every inactive icon; selected peak >= 215/255 and >= 90% of the main reference, with >= 20 bright pixels. Original PNG files remain unchanged.",
    }
    (ROOT / "journal-preview/highlight-pixels.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps({"passed": result["passed"], "captures": len(captures),
                      "failed": [capture["path"] for capture in captures if not capture["passed"]],
                      "original_defect_detected": result["original_defect_detected"],
                      "dim_selection_defect_detected": result["dim_selection_defect_detected"]}))
    raise SystemExit(0 if result["passed"] else 1)


if __name__ == "__main__":
    main()
