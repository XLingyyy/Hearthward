#!/usr/bin/env python3
"""Deterministic TASK-099 sound design; offline production tool, not game code.

Uses already-vendored CC0 recordings and original procedural DSP. No downloads,
package installs, voice, TTS, or writes outside the new audio-completion assets.
Run from any directory: python art_source/TASK-099/audio-completion-v1/render_audio.py
NumPy, SciPy and ffmpeg must already be installed. Parameters are in design.json.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import wave
import zipfile

import numpy as np
import scipy
from scipy import signal
from scipy.ndimage import gaussian_filter1d, maximum_filter1d


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SR = 48000
TAU = 2.0 * np.pi


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def db(value: float) -> float:
    return float(20 * np.log10(max(float(value), 1e-12)))


def rms(x: np.ndarray) -> float:
    return float(np.sqrt(np.mean(np.square(x))))


def unit(x: np.ndarray) -> np.ndarray:
    return x / max(rms(x), 1e-12)


def decode(path: Path, start: float | None = None, duration: float | None = None) -> np.ndarray:
    args = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(path)]
    if start is not None:
        args += ["-ss", str(start)]
    if duration is not None:
        args += ["-t", str(duration)]
    args += ["-map_metadata", "-1", "-ac", "1", "-ar", str(SR), "-f", "f64le", "pipe:1"]
    return np.frombuffer(subprocess.check_output(args), dtype="<f8").copy()


def filtered(x: np.ndarray, low: float, high: float) -> np.ndarray:
    return signal.sosfilt(signal.butter(3, [low, high], btype="bandpass", fs=SR, output="sos"), x)


def edges(x: np.ndarray, attack: float = 0.003, release: float = 0.03) -> np.ndarray:
    x = x.copy()
    a = min(round(attack * SR), len(x) // 2)
    r = min(round(release * SR), len(x) // 2)
    if a:
        x[:a] *= np.sin(np.linspace(0, np.pi / 2, a)) ** 2
    if r:
        x[-r:] *= np.cos(np.linspace(0, np.pi / 2, r)) ** 2
    return x


def add(dst: np.ndarray, part: np.ndarray, when: float, gain: float = 1.0, circular: bool = False) -> None:
    start = round(when * SR)
    if circular:
        # Every event is wrapped, including its tail: no boundary fade or silence.
        np.add.at(dst, (start + np.arange(len(part))) % len(dst), gain * part)
    else:
        end = min(start + len(part), len(dst))
        if start < len(dst) and end > start:
            dst[start:end] += gain * part[:end - start]


def colored(rng: np.random.Generator, n: int, low: float, high: float, slope: float = 0.0) -> np.ndarray:
    """Circular band-shaped noise: no IIR startup transient at a loop boundary."""
    f = np.fft.rfftfreq(n, 1 / SR)
    safe = np.maximum(f, 0.01)
    shape = (1 - np.exp(-(safe / low) ** 4)) * np.exp(-(safe / high) ** 4)
    shape *= (np.maximum(safe, low) / low) ** (-slope / 2)
    z = (rng.normal(size=len(f)) + 1j * rng.normal(size=len(f))) * shape
    z[0] = 0
    z[-1] = complex(z[-1].real, 0)
    return unit(np.fft.irfft(z, n))


def motion(rng: np.random.Generator, n: int, cycles: int, floor: float = 0.25) -> np.ndarray:
    """Smooth periodic irregular envelope from integer Fourier components."""
    t = np.arange(n) / n
    y = np.zeros(n)
    for k in range(1, cycles + 1):
        y += np.cos(TAU * k * t + rng.uniform(0, TAU)) / k ** 1.4
    y = (y - y.min()) / max(float(np.ptp(y)), 1e-12)
    return floor + (1 - floor) * y



def trim_source(x: np.ndarray) -> np.ndarray:
    threshold = max(float(np.max(np.abs(x))) * 0.016, 1e-5)
    active = np.flatnonzero(np.abs(x) > threshold)
    if not len(active):
        raise ValueError("A source recording is silent")
    return edges(x[max(0, active[0] - 100):min(len(x), active[-1] + 1500)])


def design_landing(rng: np.random.Generator, source: dict[str, np.ndarray], seconds: float) -> np.ndarray:
    n = round(seconds * SR)
    y = np.zeros(n)
    # Two distinct actual soles, separated by 28 ms, with an unpitched cloth settle.
    left = signal.resample_poly(trim_source(source["footstep03"]), 6, 5)
    right = signal.resample_poly(trim_source(source["footstep07"]), 7, 6)
    add(y, left / np.max(np.abs(left)), 0.009, 0.82)
    add(y, right / np.max(np.abs(right)), 0.037, 0.60)
    leather = trim_source(source["dropLeather"])
    add(y, leather / np.max(np.abs(leather)), 0.032, 0.19)
    cloth = filtered(trim_source(source["cloth2"]), 600, 6600)
    add(y, cloth / np.max(np.abs(cloth)), 0.13, 0.11)
    t = np.arange(n) / SR
    # Low body weight, deliberately short and inharmonic to avoid a musical hit.
    thud = (np.sin(TAU * (76 * t - 12 * t * t)) + 0.39 * np.sin(TAU * 113.7 * t + 0.8))
    thud *= (1 - np.exp(-t / 0.003)) * np.exp(-t / 0.044)
    y += 0.35 * thud
    # Small settling grit, not a second footstep or a completion chime.
    for onset in [0.063, 0.083, 0.142, 0.197, 0.253]:
        q = np.arange(round(rng.uniform(0.012, 0.029) * SR)) / SR
        grain = filtered(rng.normal(size=len(q)), 950, 8200) * np.exp(-q / 0.004)
        add(y, edges(grain, 0.0005, 0.004), onset, rng.uniform(0.016, 0.036))
    return edges(filtered(y, 30, 14000), 0.002, 0.13)


def design_water(rng: np.random.Generator, water: np.ndarray, seconds: float, entering: bool) -> np.ndarray:
    """Revision 2: real CC0 water foley, no synthesized water oscillators/noise.

    The entire output is assembled from the named recording. Source windows avoid
    pre-roll/room silence and, for exit, the large central displacement transient.
    No reversal, time stretching or pitch change; slight filtering only removes
    rumble and recording hiss. Silence padding leaves a clean event tail.
    """
    n = round(seconds * SR)
    y = np.zeros(n)
    if entering:
        # The complete original splash crest and its irregular natural drain.
        take = water[round(0.525 * SR):round(1.455 * SR)]
        take = filtered(take, 90, 9500)
        t = np.arange(len(take)) / SR
        # Revision 3: the sharp source splats are INSIDE the first 0.3 seconds,
        # not just at file onset. Warm their spectrum, then smoothly duck those
        # local crest peaks without adding or inventing new water.
        warm = signal.sosfilt(signal.butter(3, 2300, btype="lowpass", fs=SR, output="sos"), take)
        openness = 0.18 + 0.82 * np.clip((t - 0.30) / 0.24, 0, 1)
        take = warm * (1 - openness) + take * openness
        local_peak = maximum_filter1d(np.abs(take), size=481, mode="nearest")
        crest_gain = np.minimum(1.0, 0.035 / np.maximum(local_peak, 1e-12))
        crest_gain = gaussian_filter1d(crest_gain, sigma=120, mode="nearest")
        take *= crest_gain
        take = edges(take, 0.030, 0.10)
        add(y, take, 0.004)
    else:
        # Smaller wet lift/brush followed by real drain. Avoid the larger source
        # spike at 0.807s, which would make exit another full displacement impact.
        drain = water[round(0.883 * SR):round(1.670 * SR)]
        drain = edges(filtered(drain, 105, 10500), 0.012, 0.13)
        wet_motion = water[round(0.639 * SR):round(0.774 * SR)]
        wet_motion = edges(filtered(wet_motion, 110, 10000), 0.007, 0.045)
        add(y, drain, 0.006, 0.92)
        add(y, wet_motion, 0.002, 0.20)
    return edges(y, 0.001, 0.025)


def design_fire(rng: np.random.Generator, seconds: float) -> np.ndarray:
    n = round(seconds * SR)
    y = 0.034 * colored(rng, n, 45, 1050, 1.05) * motion(rng, n, 13, 0.48)
    y += 0.009 * colored(rng, n, 1000, 8500, 0.7) * motion(rng, n, 41, 0.3)
    # Small crackles arrive in irregular clusters, with independent amplitude,
    # spectral tilt, and decay. Larger wood pops contain several nonharmonic modes.
    clusters = rng.uniform(0, seconds, 31)
    onsets = [(float(c + rng.normal(0, 0.10)) % seconds) for c in clusters for _ in range(rng.integers(3, 10))]
    onsets.extend(rng.uniform(0, seconds, 39))
    for onset in onsets:
        duration = rng.uniform(0.012, 0.075)
        t = np.arange(round(duration * SR)) / SR
        pop = filtered(rng.normal(size=len(t)), rng.uniform(520, 1500), rng.uniform(6500, 12000))
        decay = rng.uniform(0.0018, 0.0075)
        pop *= (1 - np.exp(-t / 0.00009)) * np.exp(-t / decay)
        pop = edges(pop, 0.0001, 0.007)
        add(y, pop, onset, rng.uniform(0.018, 0.10), True)
    for onset in rng.uniform(0, seconds, 20):
        t = np.arange(round(0.22 * SR)) / SR
        noise = filtered(rng.normal(size=len(t)), 160, 6200) * np.exp(-t / 0.006)
        wood = np.zeros(len(t))
        for hz, amp, decay in [(rng.uniform(150, 290), 0.25, 0.023), (rng.uniform(480, 810), 0.31, 0.017), (rng.uniform(1250, 2000), 0.19, 0.012)]:
            wood += amp * np.sin(TAU * hz * t + rng.uniform(0, TAU)) * np.exp(-t / decay)
        pop = edges(noise + wood, 0.00015, 0.04)
        add(y, pop, float(onset), rng.uniform(0.10, 0.23), True)
        # A softer paired snap gives the occasional splitting-wood character.
        if rng.random() < 0.45:
            add(y, pop, float(onset + rng.uniform(0.008, 0.024)), rng.uniform(0.02, 0.065), True)
    return y


def design_wind(rng: np.random.Generator, seconds: float) -> np.ndarray:
    n = round(seconds * SR)
    # Independent bands breathe at different rates, rather than one modulated
    # white-noise sample. No oscillator produces an exposed fixed whistle tone.
    gust = motion(rng, n, 6, 0.31)
    low = 0.12 * colored(rng, n, 38, 430, 1.1) * (0.50 + 0.50 * gust)
    air = 0.105 * colored(rng, n, 160, 2600, 0.8) * gust
    leaves = 0.028 * colored(rng, n, 1550, 9800, 0.55) * motion(rng, n, 34, 0.16) * gust
    # Broad filtered turbulent resonances pass between bands with different
    # envelopes, implying air around branches without an obvious musical pitch.
    breath = np.zeros(n)
    for low_hz, high_hz in [(310, 630), (650, 1080), (1180, 1830)]:
        band = colored(rng, n, low_hz, high_hz, 0.0)
        breath += 0.010 * band * motion(rng, n, 4, 0.10)
    # Short rustling eddies are sparse, soft, and well below fire's impulses.
    eddies = np.zeros(n)
    for onset in rng.uniform(0, seconds, 24):
        dur = rng.uniform(0.10, 0.32)
        t = np.arange(round(dur * SR)) / SR
        e = np.sin(np.pi * np.arange(len(t)) / max(len(t) - 1, 1)) ** 2
        grain = filtered(rng.normal(size=len(t)), 1800, 8500) * e
        add(eddies, grain, float(onset), rng.uniform(0.005, 0.014), True)
    return low + air + leaves + breath + eddies


def finish(x: np.ndarray, peak_db: float, loop: bool) -> tuple[np.ndarray, dict]:
    x = np.asarray(x, np.float64)
    if loop:
        # Circular spectral high-pass preserves periodicity, removes DC and rumble.
        f = np.fft.rfftfreq(len(x), 1 / SR)
        z = np.fft.rfft(x)
        z *= 1 - np.exp(-(f / 25) ** 4)
        x = np.fft.irfft(z, len(x))
        # Rotate to a quiet continuous zero-crossing; the audio is already circular.
        # This does not fabricate an endpoint by splicing unrelated samples.
        d = x - np.roll(x, 1)
        e = signal.fftconvolve(x * x, np.ones(481) / 481, mode="same")
        eligible = np.flatnonzero((np.abs(d) < np.quantile(np.abs(d), 0.015)) & (np.abs(x) < rms(x) * 0.12) & (e > rms(x) ** 2 * 0.28))
        cut = int(eligible[len(eligible) // 2]) if len(eligible) else int(np.argmin(np.abs(d)))
        x = np.roll(x, -cut)
    else:
        cut = None
        x = edges(x, 0.001, 0.025)
    # Normalize against 4x oversampled true-peak estimate, leaving mix headroom.
    true_peak = float(np.max(np.abs(signal.resample_poly(np.r_[x[-128:], x, x[:128]], 4, 1))))
    x *= 10 ** (peak_db / 20) / max(true_peak, 1e-12)
    if not loop:
        x[0] = x[-1] = 0.0
    pcm = np.rint(np.clip(x, -1, 1) * 32767).astype("<i2")
    q = pcm.astype(np.float64) / 32768.0
    true_peak_q = float(np.max(np.abs(signal.resample_poly(np.r_[q[-128:], q, q[:128]], 4, 1))))
    derivative = np.diff(q)
    boundary = float(q[0] - q[-1])
    stats = {
        "sample_rate_hz": SR, "channels": 1, "encoding": "PCM_S16LE", "bits_per_sample": 16,
        "frames": len(pcm), "duration_seconds": len(pcm) / SR,
        "sample_peak_dbfs": round(db(np.max(np.abs(q))), 6),
        "estimated_true_peak_4x_dbtp": round(db(true_peak_q), 6),
        "rms_dbfs": round(db(rms(q)), 6), "dc_offset": round(float(np.mean(q)), 9),
        "full_scale_samples": int(np.count_nonzero(np.abs(pcm.astype(np.int32)) >= 32767)),
        "nonzero_samples": int(np.count_nonzero(pcm)),
        "loop": loop,
        "boundary_sample_delta_pcm16": int(pcm[0]) - int(pcm[-1]),
        "boundary_step_over_interior_derivative_rms": round(abs(boundary) / max(rms(derivative), 1e-12), 6),
        "loop_rotation_frames": cut,
    }
    if loop:
        window = SR // 10
        stats["boundary_100ms_rms_delta_db"] = round(abs(db(rms(q[:window])) - db(rms(q[-window:]))), 6)
    assert np.all(np.isfinite(q)) and len(q) > 0
    assert stats["full_scale_samples"] == 0
    return pcm, stats


def write_wav(path: Path, pcm: np.ndarray) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(SR)
        out.writeframes(pcm.astype("<i2").tobytes())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-root", type=Path, default=ROOT, help="Repository-shaped output root for non-mutating reproduction checks")
    args = parser.parse_args()
    out_root = args.output_root.resolve()
    design = json.loads((HERE / "design.json").read_text(encoding="utf-8"))
    target = out_root / "Resources/Audio/TASK-099"
    source_out = out_root / "art_source/TASK-099/audio-completion-v1"
    source_out.mkdir(parents=True, exist_ok=True)
    kenney = ROOT / "art_source/TASK-099/kenney-rpg-audio/kenney_rpg-audio.zip"
    peludo = HERE / "peludo-water-splash"
    decoded = {}
    source_records = []
    with zipfile.ZipFile(kenney) as pack:
        for name in ["footstep03", "footstep07", "dropLeather", "cloth2"]:
            member = f"Audio/{name}.ogg"
            path = source_out / "recordings" / (name + ".ogg")
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(pack.read(member))
            decoded[name] = decode(path)
            source_records.append({"path": str(path.relative_to(out_root)), "sha256": sha256(path), "archive_member": member, "license": "CC0-1.0", "author": "Kenney Vleugels / Kenney.nl"})
    # Revision 2 uses actual water-action recordings rather than river noise + DSP.
    water_enter = decode(peludo / "splash1_0.wav")
    water_exit = decode(peludo / "splash2_0.wav")
    cues = []
    rendered = {}
    for cue in design["cues"]:
        rng = np.random.default_rng(cue["seed"])
        kind = cue["kind"]
        if kind == "landing":
            raw = design_landing(rng, decoded, cue["duration_seconds"])
        elif kind in ["water_enter", "water_exit"]:
            raw = design_water(rng, water_enter if kind == "water_enter" else water_exit, cue["duration_seconds"], kind == "water_enter")
        elif kind == "fire":
            raw = design_fire(rng, cue["duration_seconds"])
        elif kind == "wind":
            raw = design_wind(rng, cue["duration_seconds"])
        else:
            raise ValueError(f"Unknown cue type: {kind}")
        pcm, stats = finish(raw, cue["target_true_peak_db"], cue["loop"])
        path = target / cue["filename"]
        write_wav(path, pcm)
        rendered[kind] = pcm
        cues.append(dict(cue, **{k: v for k, v in stats.items() if k not in cue}, path=str(path.relative_to(out_root)), sha256=sha256(path)))
    # Runtime-effective level audition; gains do not change source PCM files.
    montage, chapters = [], []
    position = 0
    for kind in ["landing", "water_enter", "water_exit", "fire", "wind"]:
        silence = np.zeros(round(0.65 * SR), dtype="<i2")
        montage.append(silence)
        position += len(silence)
        runtime_gain = {"fire": 0.35, "wind": 0.24}.get(kind, 1.0)
        piece = np.rint(rendered[kind].astype(np.float64) * runtime_gain).astype("<i2")
        if kind in ["fire", "wind"]:
            piece = np.concatenate([piece, piece[:2 * SR]])
        chapters.append({"kind": kind, "start_seconds": position / SR, "end_seconds": (position + len(piece)) / SR,
                         "loop_wrap_seconds": (position + 16 * SR) / SR if kind in ["fire", "wind"] else None, "runtime_gain": runtime_gain})
        montage.append(piece)
        position += len(piece)
    montage.append(np.zeros(round(0.5 * SR), dtype="<i2"))
    montage_path = source_out / "audition-task099-completion.wav"
    write_wav(montage_path, np.concatenate(montage))
    manifest = {
        "schema_version": 1, "task": "TASK-099", "asset_set": "audio-completion-v1", "revision": 3, "created_date": "2026-10-10",
        "source_commit": "1bdc01b642dcf6e322ddca6cd4d7ad1a3fcc6351",
        "production_method": "Revision 3: entry internal splash crests smoothed and warmed with real source tail retained; CC0 real water-action recordings edited for entry/exit without synthesized bubbles or noise; landing retains CC0/DSP composite; fire unchanged original DSP; wind same DSP reduced 6 dB.",
        "subjective_listening": "NOT_RUN: no audio-output device or human listening result is claimed by this generation report.",
        "in_engine_listening_and_shipping": "NOT_RUN by asset author; integration owns these checks.",
        "fixed_voice": "UNPRODUCED; no speech, TTS, silent voice replacement, or dialogue edits.",
        "reproduction": {
            "command": "python art_source/TASK-099/audio-completion-v1/render_audio.py",
            "python": platform.python_version(), "numpy": np.__version__, "scipy": scipy.__version__,
            "ffmpeg": subprocess.check_output(["ffmpeg", "-version"], text=True).splitlines()[0],
            "script_sha256": sha256(HERE / "render_audio.py"), "design_sha256": sha256(HERE / "design.json"),
            "random_algorithm": "NumPy Generator PCG64 with per-cue recorded seed",
            "scope": "Same tool versions and source bytes reproduce exact PCM; other decoder/library versions require comparison.",
        },
        "sources": {
            "kenney_archive": {"path": str(kenney.relative_to(ROOT)), "sha256": sha256(kenney), "url": "https://kenney.nl/assets/rpg-audio", "license": "CC0-1.0", "extracted_recordings": source_records},
            "peludo": {
                "work": "Water Splash and sand footsteps", "author": "Peludo / RNAn",
                "url": "https://opengameart.org/content/water-splash-and-sand-footsteps",
                "creator_credit_url": "https://rnan.itch.io/", "license": "CC0-1.0",
                "source_type": "Actual bucket-water experiment recordings per author; not claimed as a person entering/exiting a lake.",
                "recordings": [{"path": str((peludo / name).relative_to(ROOT)), "sha256": sha256(peludo / name),
                                "url": "https://opengameart.org/sites/default/files/" + name} for name in ["splash1_0.wav", "splash2_0.wav"]],
                "edit_windows_seconds": {"water_enter_splash1": [0.525, 1.455], "water_exit_splash2_drain": [0.883, 1.670], "water_exit_splash2_wet_motion": [0.639, 0.774]},
                "processing": "Mono 48k PCM decode, band-pass, onset/tail edges, level/headroom. Revision 3 entry additionally warms first 0.3s spectral attack and smoothly reduces local 10ms crest peaks. No added oscillators, noise, pitch shift or reversal."
            },
            "procedural": {"authoring": "New deterministic synthesis and arrangement created for Hearthward TASK-099", "third_party_samples": False, "licensing": "Project-created output; no additional third-party audio restrictions. This notice does not impose a new license on Hearthward or claim exclusive copyright in AI-assisted material."},
        },
        "cues": cues,
        "audition": {"path": str(montage_path.relative_to(out_root)), "sha256": sha256(montage_path), "duration_seconds": sum(len(x) for x in montage) / SR, "chapters": chapters, "note": "Runtime-effective presentation: water/landing gain 1.0, fire 0.35, wind 0.24; no per-segment normalization. 0.65s silence between examples. Each ambience plays 16s plus 2s after its wrap. Montage starts/stops are intentionally excerpt boundaries, not runtime loop boundaries."},
    }
    manifest_path = source_out / "cue-manifest.json"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"manifest": str(manifest_path), "cues": [{"file": c["filename"], "seconds": c["duration_seconds"], "true_peak_dbtp": c["estimated_true_peak_4x_dbtp"], "rms_dbfs": c["rms_dbfs"], "boundary_pcm_delta": c["boundary_sample_delta_pcm16"]} for c in cues]}, indent=2))


if __name__ == "__main__":
    main()
