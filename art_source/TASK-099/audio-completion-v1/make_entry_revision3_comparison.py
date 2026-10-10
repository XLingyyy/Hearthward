#!/usr/bin/env python3
"""Regenerate entry-only v2/v3 comparison at unchanged runtime gain 1.0.

Pass a preserved revision-2 root (for example its delivered changes directory).
This does not update runtime WAVs or normalize either source.
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


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-root", required=True, type=Path)
    parser.add_argument("--candidate-root", type=Path, default=ROOT, help="Revision-3 root (use a preserved root for later replays)")
    args = parser.parse_args()
    parts, chapters, at = [], [], 0
    for revision, base, gap in [("v2", args.baseline_root, .4), ("v3", args.candidate_root, .65)]:
        source = base / "Resources/Audio/TASK-099/movement-water-enter.wav"
        with wave.open(str(source), "rb") as w:
            assert (w.getframerate(), w.getsampwidth(), w.getnchannels()) == (SR, 2, 1)
            pcm = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2")
        if revision == "v2":
            assert sha(source) == "00adf5b48c744c86797e1f7c60e553324df7755918334b93730ffec575bab2c1", "Wrong revision-2 baseline"
        else:
            assert sha(source) == "045787014fabe08a434399c0b2776a66396f6feacd04311f05c240d4ef09368c", "Wrong revision-3 candidate"
        parts.append(np.zeros(round(gap * SR), dtype="<i2"))
        at += round(gap * SR)
        q = pcm.astype(np.float64) / 32768
        early = q[:14400]
        rms20 = np.sqrt(np.mean(early.reshape(-1, 960) ** 2, axis=1))
        rms = np.sqrt(np.mean(q * q))
        chapters.append({"revision": revision, "start_seconds": at / SR,
                         "end_seconds": (at + len(pcm)) / SR, "runtime_gain": 1.0,
                         "source_sha256": sha(source), "sample_peak": float(np.max(np.abs(q))),
                         "rms_dbfs": float(20 * np.log10(rms)),
                         "crest_db": float(20 * np.log10(np.max(np.abs(q)) / rms)),
                         "first_300ms_max_20ms_rms": float(np.max(rms20)),
                         "first_300ms_peak": float(np.max(np.abs(early)))})
        parts.append(pcm)
        at += len(pcm)
    parts.append(np.zeros(round(.59 * SR), dtype="<i2"))
    out = HERE / "comparison-entry-v2-v3-runtime-gain.wav"
    with wave.open(str(out), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(np.concatenate(parts).tobytes())
    meta = {"task": "TASK-099", "revision": 3, "path": str(out.relative_to(ROOT)),
            "sha256": sha(out), "duration_seconds": sum(len(p) for p in parts) / SR,
            "chapters": chapters,
            "normalization": "NONE: both use actual runtime gain 1.0; same PCM source bytes as the corresponding runtime file.",
            "changes": "Entry only: warm and smooth internal first-quarter-second source splat crests, preserve recorded water tail; new source true-peak ceiling -16dBTP prevents peak normalization from restoring the old attack. No new sound layers.",
            "listening_status": "Prepared for user review; no subjective listening claim."}
    (HERE / "comparison-entry-v2-v3-runtime-gain.json").write_text(json.dumps(meta, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"path": str(out), "sha256": sha(out), "duration_seconds": meta["duration_seconds"]}))


if __name__ == "__main__":
    main()
