#!/usr/bin/env python3
"""Make the requested unnormalized A/B with the same actual runtime gains.

The baseline root must contain the preserved revision-1 files. No source file is
changed. Water is gain 1.0; wind is gain 0.24 in BOTH A and B, same 0-6s window.
"""
import argparse
import hashlib
import json
from pathlib import Path
import wave

import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SR = 48000


def read(path):
    with wave.open(str(path), "rb") as w:
        assert (w.getnchannels(), w.getsampwidth(), w.getframerate()) == (1, 2, SR)
        return np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").copy()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-root", type=Path, required=True)
    parser.add_argument("--candidate-root", type=Path, default=ROOT, help="Preserved revision-2 root; current root is valid only while it is revision 2")
    args = parser.parse_args()
    order = [
        ("wind", "old", "environment-wind-loop.wav", .24, 6.0, .50),
        ("wind", "new", "environment-wind-loop.wav", .24, 6.0, .50),
        ("water_enter", "old", "movement-water-enter.wav", 1.0, None, .65),
        ("water_enter", "new", "movement-water-enter.wav", 1.0, None, .50),
        ("water_exit", "old", "movement-water-exit.wav", 1.0, None, .65),
        ("water_exit", "new", "movement-water-exit.wav", 1.0, None, .50),
    ]
    output = []
    chapters = []
    at = 0
    for kind, revision, name, gain, seconds, gap in order:
        root = args.baseline_root if revision == "old" else args.candidate_root
        path = root / "Resources/Audio/TASK-099" / name
        expected = {
            ("wind", "old"): "e80d78d96f4278bc595dbe3da34df01350aaad7b3c476a9e70492c7e5ff78b66",
            ("wind", "new"): "0f5176009a9dc2c44dab69827b6375954efbbeb6f10b42a0f00b780c416cadaf",
            ("water_enter", "old"): "5639cb6408f2f36d12c987de9e2a7b66c7bf00a0d657e0182ff7778835240504",
            ("water_enter", "new"): "00adf5b48c744c86797e1f7c60e553324df7755918334b93730ffec575bab2c1",
            ("water_exit", "old"): "d845ae29ff2182eabed3107ba6ac30062f6f2282824518509562950990cd2ec0",
            ("water_exit", "new"): "07a1ffbdc0d75333d92e7d7d769f78466e383739e118bb665943396a3fd9cba7",
        }
        assert sha(path) == expected[(kind, revision)], "Wrong source revision; pass the preserved revision 1 and 2 roots"
        p = read(path)
        if seconds is not None:
            p = p[:round(seconds * SR)]
        q = np.rint(p.astype(np.float64) * gain).astype("<i2")
        # Same brief fades only at montage wind excerpt boundaries; preserve
        # interior level exactly and do not touch the actual runtime loops.
        if kind == "wind":
            e = np.ones(len(q))
            e[:240] = np.linspace(0, 1, 240)
            e[-240:] = np.linspace(1, 0, 240)
            q = np.rint(q.astype(np.float64) * e).astype("<i2")
        silence = np.zeros(round(gap * SR), dtype="<i2")
        output.append(silence)
        at += len(silence)
        chapters.append({"kind": kind, "revision": revision, "start_seconds": at / SR,
                         "end_seconds": (at + len(q)) / SR, "runtime_gain": gain,
                         "source_window_start_seconds": 0.0, "source_window_duration_seconds": len(q) / SR,
                         "source_file": name, "source_sha256": sha(path),
                         "rms_dbfs_at_runtime_gain": float(20 * np.log10(np.sqrt(np.mean((q.astype(np.float64) / 32768) ** 2))))})
        output.append(q)
        at += len(q)
    output.append(np.zeros(SR // 2, dtype="<i2"))
    path = HERE / "comparison-v1-v2-runtime-gain.wav"
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(np.concatenate(output).tobytes())
    meta = {"task": "TASK-099", "revision": 2, "path": str(path.relative_to(ROOT)), "sha256": sha(path),
            "duration_seconds": sum(len(x) for x in output) / SR, "channels": 1, "sample_rate_hz": SR,
            "encoding": "PCM_S16LE", "chapters": chapters,
            "normalization": "NONE; both revisions use the identical runtime gain for the compared cue. Wind source is 6 dB lower in revision 2.",
            "master_and_category_gain": "1.0 for comparison; actual user settings can scale the game mix further.",
            "wind_windows": "Same 0-6s source timeline. Equal 5ms montage-only fades prevent excerpt clicks.",
            "voice_labels": "None; use the supplied timestamp labels.",
            "listening_status": "Prepared for user A/B; no author device or subjective listening claim."}
    (HERE / "comparison-v1-v2-runtime-gain.json").write_text(json.dumps(meta, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(meta, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
