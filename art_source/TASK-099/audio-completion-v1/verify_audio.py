#!/usr/bin/env python3
"""Independent file/level/boundary/reproduction checks for the five new cues.

These checks do not listen, launch Unreal, run a sound device, or approve timbre.
Outputs a machine-readable report beside the editable audio source.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import wave

import numpy as np
from scipy import signal


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rms(x: np.ndarray) -> float:
    return float(np.sqrt(np.mean(x * x)))


def db(x: float) -> float:
    return float(20 * np.log10(max(x, 1e-12)))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-reproduction", action="store_true", help="Skip the clean isolated rerender; report it as NOT_RUN")
    args = parser.parse_args()
    manifest = json.loads((HERE / "cue-manifest.json").read_text(encoding="utf-8"))
    checks = []
    metrics = []
    pcm_by_kind = {}

    def check(label: str, passed: bool, details=None) -> None:
        checks.append({"check": label, "status": "PASS" if passed else "FAIL", "details": details})

    check("cue count", len(manifest["cues"]) == 5)
    check("unique files", len({c["sha256"] for c in manifest["cues"]}) == 5)
    check("generator provenance", digest(HERE / "render_audio.py") == manifest["reproduction"]["script_sha256"])
    check("design provenance", digest(HERE / "design.json") == manifest["reproduction"]["design_sha256"])
    for cue in manifest["cues"]:
        path = ROOT / cue["path"]
        with wave.open(str(path), "rb") as wav:
            params = wav.getparams()
            p = np.frombuffer(wav.readframes(params.nframes), dtype="<i2").copy()
        q = p.astype(np.float64) / 32768.0
        key = cue["kind"]
        pcm_by_kind[key] = q
        check(key + ": RIFF/PCM mono 48k16", params.nchannels == 1 and params.framerate == 48000 and params.sampwidth == 2 and params.comptype == "NONE", list(params))
        check(key + ": frame count", params.nframes == round(cue["duration_seconds"] * 48000))
        check(key + ": manifest hash", digest(path) == cue["sha256"])
        check(key + ": non-silent", rms(q) > 0.004 and np.count_nonzero(p) > len(p) * 0.45)
        check(key + ": no PCM clipping", not np.any(np.abs(p.astype(np.int32)) >= 32767))
        tp = np.max(np.abs(signal.resample_poly(np.r_[q[-128:], q, q[:128]], 4, 1)))
        check(key + ": true-peak headroom", db(float(tp)) <= cue["target_true_peak_db"] + 0.002, db(float(tp)))
        check(key + ": DC offset", abs(float(q.mean())) < 0.0001, float(q.mean()))
        frequencies, power = signal.welch(q, fs=48000, nperseg=4096)
        bands = {}
        for name, low, high in [("bass_25_250_hz", 25, 250), ("body_250_2000_hz", 250, 2000), ("detail_2000_12000_hz", 2000, 12000)]:
            bands[name] = round(float(power[(frequencies >= low) & (frequencies < high)].sum() / power.sum()), 6)
        check(key + ": multiband spectral content", sum(v > 0.005 for v in bands.values()) >= 2, bands)
        active = np.flatnonzero(p != 0)
        met = {"kind": key, "sha256": digest(path), "duration_seconds": len(p) / 48000, "peak_dbfs": db(float(np.abs(q).max())), "estimated_true_peak_4x_dbtp": db(float(tp)), "rms_dbfs": db(rms(q)), "spectral_energy_fractions": bands}
        if cue["loop"]:
            delta = int(p[0]) - int(p[-1])
            normalized_step = abs(delta / 32768.0) / rms(np.diff(q))
            check(key + ": near-continuous sample boundary", abs(delta) <= 12 and normalized_step < 0.10, {"pcm16_delta": delta, "step_over_interior_derivative_rms": normalized_step})
            windows = q.reshape(-1, 480)
            lowest_window = float(np.min(np.sqrt(np.mean(windows * windows, axis=1))))
            check(key + ": no silent loop/fade gap", lowest_window > 0.001, {"minimum_10ms_rms_dbfs": db(lowest_window)})
            left = rms(q[-4800:])
            right = rms(q[:4800])
            check(key + ": boundary energy neighborhood", abs(db(left) - db(right)) < 6.0, {"100ms_level_change_db": abs(db(left) - db(right))})
            # A real stream of three periods keeps both repeated joins identical.
            three = np.tile(p, 3)
            check(key + ": three-period PCM continuity", all(int(three[n]) - int(three[n - 1]) == delta for n in [len(p), 2 * len(p)]))
            met.update({"boundary_pcm16_delta": delta, "minimum_10ms_rms_dbfs": db(lowest_window), "three_period_frames": len(three)})
        else:
            check(key + ": leading event latency", int(active[0]) < 0.02 * 48000, {"first_nonzero_seconds": float(active[0] / 48000)})
            check(key + ": clean tail endpoints", p[0] == 0 and p[-1] == 0)
            met["first_nonzero_seconds"] = float(active[0] / 48000)
            met["last_nonzero_seconds"] = float(active[-1] / 48000)
        metrics.append(met)

    common = min(len(pcm_by_kind["water_enter"]), len(pcm_by_kind["water_exit"]))
    correlation = float(np.corrcoef(pcm_by_kind["water_enter"][:common], pcm_by_kind["water_exit"][:common])[0, 1])
    check("water entry and exit are distinct waveforms", abs(correlation) < 0.80, {"same_time_correlation": correlation})
    for source in [manifest["sources"]["kenney_archive"]] + manifest["sources"]["kenney_archive"]["extracted_recordings"] + manifest["sources"]["peludo"]["recordings"]:
        check("source integrity: " + source["path"], digest(ROOT / source["path"]) == source["sha256"])
    license_path = ROOT / "Resources/Audio/TASK-099/License-Audio-Completion-v1.txt"
    check("runtime attribution/notice exists", license_path.is_file() and license_path.stat().st_size > 1000)
    events = json.loads((ROOT / "Resources/Data/experience.json").read_text(encoding="utf-8"))["sound_events"]
    for cue in manifest["cues"]:
        matching = [e for e in events if e["event_id"] == cue["event_id"]]
        check("runtime mapping: " + cue["event_id"], len(matching) == 1 and matching[0]["file"] == "TASK-099/" + cue["filename"])
    check("revision 3 actual CC0 water provenance", manifest["revision"] == 3 and manifest["sources"]["peludo"]["license"] == "CC0-1.0")
    check("runtime Peludo attribution exists", (ROOT / "Resources/Audio/TASK-099/License-Peludo-Water-Splash.txt").is_file())
    for cue in manifest["cues"]:
        check("runtime-effective preview gain: " + cue["kind"], cue["audition_runtime_gain"] == {"fire": .35, "wind": .24}.get(cue["kind"], 1.0))
    expected_unchanged = {"landing": "7664890ca057e7a2a24ebe3b5fb2f5fd9c2f2f73e5f16f71ce7d03dfdd485c92", "fire": "7d4833b73af34f84728b81ff2254f4c2743161ce566a5d7c668b98bd6ca20805"}
    for cue in manifest["cues"]:
        if cue["kind"] in expected_unchanged:
            check("revision 1 source unchanged: " + cue["kind"], digest(ROOT / cue["path"]) == expected_unchanged[cue["kind"]])
    check("wind revision gain change target", next(c for c in manifest["cues"] if c["kind"] == "wind")["target_true_peak_db"] == -16.0)
    comparison = json.loads((HERE / "comparison-v1-v2-runtime-gain.json").read_text(encoding="utf-8"))
    check("comparison integrity", digest(ROOT / comparison["path"]) == comparison["sha256"])
    wind_ab = [c for c in comparison["chapters"] if c["kind"] == "wind"]
    check("wind A/B equal gains and time window", len(wind_ab) == 2 and all(c["runtime_gain"] == .24 and c["source_window_start_seconds"] == 0.0 and c["source_window_duration_seconds"] == 6.0 for c in wind_ab))
    check("wind A/B measured 6 dB reduction", abs((wind_ab[1]["rms_dbfs_at_runtime_gain"] - wind_ab[0]["rms_dbfs_at_runtime_gain"]) + 6.0) < .001)
    for chapter in comparison["chapters"]:
        if chapter["revision"] == "new" and chapter["kind"] != "water_enter":
            check("historical v2 unchanged source matches current: " + chapter["kind"], digest(ROOT / "Resources/Audio/TASK-099" / chapter["source_file"]) == chapter["source_sha256"])
    entry_ab = json.loads((HERE / "comparison-entry-v2-v3-runtime-gain.json").read_text(encoding="utf-8"))
    check("revision 3 entry comparison integrity", digest(ROOT / entry_ab["path"]) == entry_ab["sha256"])
    v2, v3 = entry_ab["chapters"]
    check("entry comparison equal runtime gain", v2["runtime_gain"] == v3["runtime_gain"] == 1.0)
    check("entry comparison current source", digest(ROOT / "Resources/Audio/TASK-099/movement-water-enter.wav") == v3["source_sha256"])
    check("entry comparison correct v2 baseline", v2["source_sha256"] == "00adf5b48c744c86797e1f7c60e553324df7755918334b93730ffec575bab2c1")
    check("entry crest materially softened", v3["crest_db"] < v2["crest_db"] - 6.0)
    check("entry first 300ms impulse peak reduced", v3["first_300ms_peak"] < v2["first_300ms_peak"] * .5)
    check("entry first 300ms 20ms burst reduced", v3["first_300ms_max_20ms_rms"] < v2["first_300ms_max_20ms_rms"] * .9)
    check("revision 2 exit unchanged", digest(ROOT / "Resources/Audio/TASK-099/movement-water-exit.wav") == "07a1ffbdc0d75333d92e7d7d769f78466e383739e118bb665943396a3fd9cba7")
    check("revision 2 wind unchanged", digest(ROOT / "Resources/Audio/TASK-099/environment-wind-loop.wav") == "0f5176009a9dc2c44dab69827b6375954efbbeb6f10b42a0f00b780c416cadaf")
    audition = ROOT / manifest["audition"]["path"]
    check("audition montage integrity", digest(audition) == manifest["audition"]["sha256"])

    reproduction = "NOT_RUN"
    if not args.skip_reproduction:
        with tempfile.TemporaryDirectory(prefix="hearthward-task099-audio-verify-") as folder:
            run = subprocess.run([sys.executable, str(HERE / "render_audio.py"), "--output-root", folder], capture_output=True, text=True)
            check("isolated deterministic rerender command", run.returncode == 0, {"exit_code": run.returncode, "stderr": run.stderr})
            if run.returncode == 0:
                for cue in manifest["cues"]:
                    check("exact rerender bytes: " + cue["filename"], digest(Path(folder) / cue["path"]) == cue["sha256"])
                check("exact rerender bytes: audition", digest(Path(folder) / manifest["audition"]["path"]) == manifest["audition"]["sha256"])
                reproduction = "PASS" if all(c["status"] == "PASS" for c in checks if "rerender" in c["check"]) else "FAIL"

    report = {"schema_version": 1, "task": "TASK-099", "date": "2026-10-10", "source_commit": manifest["source_commit"],
              "command": "python art_source/TASK-099/audio-completion-v1/verify_audio.py" + (" --skip-reproduction" if args.skip_reproduction else ""),
              "status": "PASS" if all(c["status"] == "PASS" for c in checks) else "FAIL", "check_count": len(checks),
              "pass_count": sum(c["status"] == "PASS" for c in checks), "checks": checks, "metrics": metrics,
              "reproduction": reproduction,
              "not_run": ["Sound-device or human subjective listening", "Owner timbre/material/mix acceptance", "In-engine event sync/attenuation/pause lifecycle", "Normal-play route recording", "Current Shipping/Cook playback"],
              "scope": "Only the five new asset files and their source/manifest/runtimemapping evidence; no prior engine PASS is imported."}
    (HERE / "validation.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: report[k] for k in ["status", "check_count", "pass_count", "reproduction"]}, indent=2))
    for c in checks:
        if c["status"] == "FAIL":
            print(json.dumps(c, ensure_ascii=False))
    if report["status"] != "PASS":
        raise SystemExit(1)


if __name__ == "__main__":
    main()
