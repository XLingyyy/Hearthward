#!/usr/bin/env python3
"""Generate TASK-104 original boot Foley; only Python's standard library is used.

No recording, downloaded sample, previous game sound, or trained audio model is
an input. Each surface has its own synthesis recipe, with independently seeded
heel, toe, friction and material-grain events. See art_source/TASK-104/footstep-audio.md.

    python scripts/audio/generate_surface_footsteps.py
    python scripts/audio/generate_surface_footsteps.py --check

--check regenerates in memory and verifies every WAV and the provenance manifest
byte-for-byte. It does not write files. Source edits require regeneration because
the manifest also records this generator's SHA-256.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import random
import struct
import sys
import wave

SAMPLE_RATE = 44100
SURFACES = ("grass", "dirt", "stone", "wood", "unknown")
VARIANTS = 3
ROOT = Path(__file__).resolve().parents[2]
OUTPUT_DIR = ROOT / "Resources/Audio/TASK-104"
GENERATOR_VERSION = 1
RIGHTS_STATUS = "PROJECT_TERMS_NO_SEPARATE_GRANT"
REVIEW_PATH = ROOT / "art_source/TASK-104/footstep-review.wav"
TAU = 2.0 * math.pi

RECIPES = {
    "grass": "Damped sole pressure, two broad blade-rustle envelopes, sparse dry leaf micro-crackles.",
    "dirt": "Packed-soil compression, irregular low-mid grit clusters, short toe drag.",
    "stone": "Hard sole impacts, brief bright contact chips, low body, sparse rough-stone scrape; no ringing metal.",
    "wood": "Hard sole contact exciting independently damped inharmonic plank modes and a short grain rub.",
    "unknown": "Neutral muted boot heel/toe impacts with leather/sole friction; its own conservative fallback recipe.",
}


def uniform(rng: random.Random, low: float, high: float) -> float:
    # random() is used directly: no platform-dependent distributions/dependencies.
    return low + (high - low) * rng.random()


def band_noise(rng: random.Random, count: int, low: float, high: float) -> list[float]:
    """Two low-pass poles plus a DC-blocking high-pass, with filter warm-up."""
    upper = 1.0 - math.exp(-TAU * high / SAMPLE_RATE)
    lower = 1.0 - math.exp(-TAU * low / SAMPLE_RATE)
    lp1 = lp2 = low_state = 0.0
    warmup = 512
    samples = []
    for i in range(count + warmup):
        value = 2.0 * rng.random() - 1.0
        lp1 += upper * (value - lp1)
        lp2 += upper * (lp1 - lp2)
        low_state += lower * (lp2 - low_state)
        if i >= warmup:
            samples.append(lp2 - low_state)
    rms = math.sqrt(sum(x * x for x in samples) / max(1, count))
    scale = 0.30 / max(1e-12, rms)
    return [x * scale for x in samples]


def noise_burst(out: list[float], rng: random.Random, start: float, duration: float,
                low: float, high: float, gain: float, attack: float,
                decay: float) -> None:
    """Rounded nonperiodic contact: exponential body and smooth finite release."""
    count = max(2, round(duration * SAMPLE_RATE))
    first = round(start * SAMPLE_RATE)
    samples = band_noise(rng, count, low, high)
    release = min(0.006, duration * 0.25)
    for i, value in enumerate(samples):
        index = first + i
        if not 0 <= index < len(out):
            continue
        t = i / SAMPLE_RATE
        envelope = (1.0 - math.exp(-t / attack)) * math.exp(-t / decay)
        remaining = (count - 1 - i) / SAMPLE_RATE
        if remaining < release:
            envelope *= math.sin(0.5 * math.pi * remaining / release) ** 2
        out[index] += gain * envelope * value


def mode(out: list[float], start: float, frequency: float, gain: float,
         decay: float, attack: float = 0.0008) -> None:
    """One damped body mode, not a sustained musical oscillator."""
    first = round(start * SAMPLE_RATE)
    count = min(len(out) - first, round(decay * 9.0 * SAMPLE_RATE))
    for i in range(max(0, count)):
        t = i / SAMPLE_RATE
        envelope = (1.0 - math.exp(-t / attack)) * math.exp(-t / decay)
        # Smooth the tiny remaining mode tail instead of cutting it abruptly.
        tail = min(1.0, (count - 1 - i) / max(1, round(0.006 * SAMPLE_RATE)))
        out[first + i] += gain * envelope * math.sin(TAU * frequency * t) * tail * tail


def sole_body(out: list[float], rng: random.Random, heel: float, toe: float,
              gain: float, softness: float) -> None:
    """Shared boot mechanics only; the material models below are independent."""
    for start, weight in ((heel, 1.0), (toe, uniform(rng, 0.49, 0.70))):
        noise_burst(out, rng, start, 0.105, 38, 270 / softness,
                    gain * weight, 0.0014 * softness, 0.020 * softness)
        mode(out, start, uniform(rng, 73, 98), gain * 0.15 * weight,
             0.017 * softness, 0.0015)


def grass(out: list[float], rng: random.Random, heel: float, toe: float) -> None:
    sole_body(out, rng, heel, toe, 0.88, 1.25)
    # Blade bending/brush is wide and soft rather than gravel-like sharp grit.
    noise_burst(out, rng, heel + 0.004, 0.169, 1000, 6800, 0.31, 0.009, 0.057)
    noise_burst(out, rng, toe + 0.003, 0.126, 1500, 7500, 0.23, 0.013, 0.039)
    for _ in range(11):
        start = heel + uniform(rng, 0.012, 0.119)
        noise_burst(out, rng, start, uniform(rng, 0.005, 0.013),
                    uniform(rng, 1300, 2300), uniform(rng, 4400, 7300),
                    uniform(rng, 0.045, 0.13), 0.0008, uniform(rng, 0.0015, 0.003))


def dirt(out: list[float], rng: random.Random, heel: float, toe: float) -> None:
    sole_body(out, rng, heel, toe, 0.79, 0.94)
    noise_burst(out, rng, heel + 0.002, 0.105, 260, 3100, 0.38, 0.003, 0.030)
    noise_burst(out, rng, toe + 0.006, 0.095, 430, 3400, 0.26, 0.006, 0.029)
    # Independent grains are displaced in two unequal heel/toe clusters.
    for contact, number, weight in ((heel, 13, 1.0), (toe, 9, 0.72)):
        for _ in range(number):
            start = contact + uniform(rng, 0.003, 0.052)
            noise_burst(out, rng, start, uniform(rng, 0.003, 0.009),
                        uniform(rng, 350, 850), uniform(rng, 1700, 4400),
                        weight * uniform(rng, 0.10, 0.28), 0.0004,
                        uniform(rng, 0.0009, 0.0025))


def stone(out: list[float], rng: random.Random, heel: float, toe: float) -> None:
    sole_body(out, rng, heel, toe, 0.73, 0.73)
    for start, weight in ((heel, 1.0), (toe, uniform(rng, 0.54, 0.73))):
        noise_burst(out, rng, start, 0.039, 750, 7200, 0.83 * weight, 0.0003, 0.005)
        noise_burst(out, rng, start + 0.001, 0.061, 160, 1900,
                    0.56 * weight, 0.0007, 0.012)
        # Short inharmonic body modes; stone does not ring like a bell or metal.
        for frequency, gain in ((177, 0.09), (327, 0.043)):
            mode(out, start, frequency * uniform(rng, 0.94, 1.07),
                 gain * weight, uniform(rng, 0.004, 0.009))
    noise_burst(out, rng, toe + 0.013, 0.075, 700, 4000, 0.12, 0.006, 0.016)
    for _ in range(4):
        noise_burst(out, rng, heel + uniform(rng, 0.011, 0.036), 0.003,
                    1900, 6800, uniform(rng, 0.05, 0.12), 0.00025, 0.0006)


def wood(out: list[float], rng: random.Random, heel: float, toe: float) -> None:
    # A flexible plank transfers more energy to a hollow, nonharmonic body.
    for start, weight in ((heel, 1.0), (toe, uniform(rng, 0.49, 0.69))):
        noise_burst(out, rng, start, 0.050, 160, 5100,
                    0.68 * weight, 0.0006, 0.011)
        for frequency, gain, decay in ((122, 0.17, 0.033), (237, 0.095, 0.026),
                                       (421, 0.057, 0.019), (783, 0.030, 0.013)):
            mode(out, start, frequency * uniform(rng, 0.94, 1.07),
                 gain * weight * uniform(rng, 0.80, 1.12),
                 decay * uniform(rng, 0.84, 1.15))
    noise_burst(out, rng, toe + 0.008, 0.081, 850, 3900, 0.13, 0.005, 0.021)


def unknown(out: list[float], rng: random.Random, heel: float, toe: float) -> None:
    # Generic outsole and leather. Avoid imposing a particular ground material.
    sole_body(out, rng, heel, toe, 0.88, 1.05)
    noise_burst(out, rng, heel, 0.069, 180, 2200, 0.36, 0.0018, 0.018)
    noise_burst(out, rng, toe, 0.066, 270, 2500, 0.25, 0.0028, 0.018)
    noise_burst(out, rng, toe + 0.011, 0.078, 440, 3100, 0.11, 0.009, 0.025)


SYNTHESIZERS = {"grass": grass, "dirt": dirt, "stone": stone, "wood": wood, "unknown": unknown}
DURATIONS = {"grass": 0.285, "dirt": 0.255, "stone": 0.220, "wood": 0.290, "unknown": 0.245}
PEAKS = {"grass": 0.53, "dirt": 0.56, "stone": 0.60, "wood": 0.58, "unknown": 0.51}


def render(surface: str, variant: int) -> tuple[list[int], dict]:
    """Render one independently seeded take, with a finite silent-safe boundary."""
    seed = 104000 + 100 * SURFACES.index(surface) + variant
    rng = random.Random(seed)
    duration = DURATIONS[surface] + uniform(rng, -0.008, 0.012)
    out = [0.0] * round(duration * SAMPLE_RATE)
    heel = uniform(rng, 0.005, 0.008)
    toe = heel + uniform(rng, 0.042, 0.070)
    SYNTHESIZERS[surface](out, rng, heel, toe)
    # Remove subsonic/DC buildup without normalizing away the surface envelopes.
    dc = 0.0
    coefficient = 1.0 - math.exp(-TAU * 28.0 / SAMPLE_RATE)
    for i, value in enumerate(out):
        dc += coefficient * (value - dc)
        out[i] = value - dc
    # Rounded fade plus 3 ms exact-zero tail: safe even for repeated runtime play.
    tail_silence = round(0.003 * SAMPLE_RATE)
    fade = round(0.016 * SAMPLE_RATE)
    weights = []
    end = len(out) - tail_silence - 1
    for i in range(len(out)):
        position = max(0.0, min(1.0, i / 100, (end - i) / fade))
        weights.append(math.sin(position * math.pi * 0.5) ** 2)
    mean = sum(x * w for x, w in zip(out, weights)) / sum(weights)
    out = [(x - mean) * w for x, w in zip(out, weights)]
    scale = PEAKS[surface] * uniform(rng, 0.94, 1.0) / max(abs(x) for x in out)
    pcm = [round(x * scale * 32767) for x in out]
    # Quantization can leave a fractional-LSB average. Correct its integer sum
    # with one-LSB interior changes, never changing silent clip boundaries.
    residual = sum(pcm)
    direction = 1 if residual > 0 else -1
    candidates = [i for i in range(256, len(pcm) - fade - tail_silence) if pcm[i] != 0]
    for j in range(abs(residual)):
        pcm[candidates[j % len(candidates)]] -= direction
    return pcm, {"seed": seed, "heel_seconds": round(heel, 6), "toe_seconds": round(toe, 6)}


def wav_bytes(pcm: list[int]) -> bytes:
    with io.BytesIO() as stream:
        with wave.open(stream, "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)
            wav.setframerate(SAMPLE_RATE)
            wav.writeframes(struct.pack(f"<{len(pcm)}h", *pcm))
        return stream.getvalue()


def audio_statistics(pcm: list[int]) -> dict:
    rms = math.sqrt(sum(x * x for x in pcm) / len(pcm)) / 32768
    peak = max(abs(x) for x in pcm) / 32768
    threshold = max(abs(x) for x in pcm) * 0.01
    active = [i for i, x in enumerate(pcm) if abs(x) >= threshold]
    return {
        "frames": len(pcm),
        "duration_seconds": round(len(pcm) / SAMPLE_RATE, 6),
        "peak_dbfs": round(20 * math.log10(peak), 3),
        "rms_dbfs": round(20 * math.log10(rms), 3),
        "dc_offset_lsb": sum(pcm) / len(pcm),
        "clipped_samples": sum(x <= -32768 or x >= 32767 for x in pcm),
        "first_sample": pcm[0], "last_sample": pcm[-1],
        "end_3ms_max_abs_lsb": max(abs(x) for x in pcm[-round(0.003 * SAMPLE_RATE):]),
        "active_above_minus40db_seconds": round((active[-1] - active[0] + 1) / SAMPLE_RATE, 6),
    }


def validate_pcm(pcm: list[int], name: str) -> None:
    stats = audio_statistics(pcm)
    checks = {
        "duration 0.18-0.34s": 0.18 < stats["duration_seconds"] < 0.34,
        "peak headroom": -7.0 < stats["peak_dbfs"] < -4.0,
        "audible body": -32.0 < stats["rms_dbfs"] < -13.0,
        "no DC offset": stats["dc_offset_lsb"] == 0,
        "no clipping": stats["clipped_samples"] == 0,
        "zero boundaries": pcm[0] == pcm[-1] == 0,
        "silent last 3ms": stats["end_3ms_max_abs_lsb"] == 0,
    }
    failed = [key for key, passed in checks.items() if not passed]
    if failed:
        raise ValueError(f"{name}: {', '.join(failed)}; {stats}")


def make_review(outputs: dict[str, bytes]) -> tuple[bytes, list[dict]]:
    """Labelled in documentation, unprocessed one-shot audition sequence."""
    pcm = [0] * round(0.5 * SAMPLE_RATE)
    cues = []
    for surface in ("unknown", "grass", "dirt", "stone", "wood"):
        for variant in range(1, VARIANTS + 1):
            name = f"footstep-{surface}-{variant:02d}.wav"
            with wave.open(io.BytesIO(outputs[name]), "rb") as wav:
                frames = wav.readframes(wav.getnframes())
            clip = struct.unpack(f"<{len(frames) // 2}h", frames)
            start = len(pcm)
            pcm.extend(clip)
            cues.append({"file": name, "start_frame": start,
                         "start_seconds": round(start / SAMPLE_RATE, 6),
                         "end_seconds": round(len(pcm) / SAMPLE_RATE, 6)})
            pcm.extend([0] * round((0.9 if variant == VARIANTS else 0.5) * SAMPLE_RATE))
    return wav_bytes(pcm), cues


def make_outputs() -> tuple[dict[str, bytes], bytes]:
    outputs = {}
    assets = []
    for surface in SURFACES:
        for variant in range(1, VARIANTS + 1):
            name = f"footstep-{surface}-{variant:02d}.wav"
            pcm, parameters = render(surface, variant)
            validate_pcm(pcm, name)
            data = wav_bytes(pcm)
            outputs[name] = data
            assets.append({"file": name, "surface": surface, "variant": variant,
                           **parameters, "sha256": hashlib.sha256(data).hexdigest(),
                           "bytes": len(data), **audio_statistics(pcm)})
    hashes = [asset["sha256"] for asset in assets]
    if len(set(hashes)) != len(hashes):
        raise ValueError("Duplicate audio assets")
    review, review_cues = make_review(outputs)
    manifest = {
        "schema_version": 1,
        "task": "TASK-104",
        "title": "Original procedural boot surface footsteps",
        "created_date": "2026-10-09",
        "origin": "Original mathematical/noise synthesis authored for Hearthward TASK-104 with coding-assistant assistance.",
        "third_party_audio_inputs": [],
        "asset_status": "ORIGINAL_PROCEDURAL",
        "rights_status": RIGHTS_STATUS,
        "rights_file": "LICENSE-PROVENANCE.txt",
        "rights_note": "No separate license grant or public-domain dedication is made. Project rights and terms apply. No third-party audio sample license is required because no such samples are inputs.",
        "generator": "scripts/audio/generate_surface_footsteps.py",
        "generator_version": GENERATOR_VERSION,
        "generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "regenerate": "python scripts/audio/generate_surface_footsteps.py",
        "verify": "python scripts/audio/generate_surface_footsteps.py --check",
        "format": {"container": "RIFF/WAVE", "encoding": "PCM signed 16-bit little-endian",
                   "sample_rate_hz": SAMPLE_RATE, "channels": 1},
        "surface_recipes": RECIPES,
        "validation_scope": "Exact reproducibility and numerical waveform checks only. UE playback and scene mixing remain unverified; human preview acceptance is recorded separately in footstep-audio.md.",
        "assets": assets,
        "review": {"file": REVIEW_PATH.relative_to(ROOT).as_posix(),
                   "runtime_asset": False,
                   "sha256": hashlib.sha256(review).hexdigest(),
                   "bytes": len(review),
                   "description": "Unprocessed runtime WAVs concatenated unknown, grass, dirt, stone, wood; variants 01-03. 0.5s leading and between-variant gaps; 0.9s between-surface and final gaps.",
                   "cues": review_cues},
    }
    outputs["PROVENANCE.json"] = (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    return outputs, review


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="Verify checked-in files byte-for-byte; never write.")
    args = parser.parse_args()
    outputs, review = make_outputs()
    if args.check:
        failures = [name for name, data in outputs.items()
                    if not (OUTPUT_DIR / name).is_file() or (OUTPUT_DIR / name).read_bytes() != data]
        expected = {name for name in outputs if name.endswith(".wav")}
        unexpected = sorted(p.name for p in OUTPUT_DIR.glob("*.wav") if p.name not in expected)
        if not (OUTPUT_DIR / "LICENSE-PROVENANCE.txt").is_file():
            failures.append("LICENSE-PROVENANCE.txt (missing)")
        if not REVIEW_PATH.is_file() or REVIEW_PATH.read_bytes() != review:
            failures.append("footstep-review.wav (missing/stale)")
        if failures or unexpected:
            print("FAIL: missing/stale files: " + ", ".join(failures or ["none"]), file=sys.stderr)
            if unexpected:
                print("FAIL: unexpected WAV files: " + ", ".join(unexpected), file=sys.stderr)
            return 1
        print(f"PASS: {len(expected)} distinct PCM16 mono {SAMPLE_RATE} Hz WAVs, provenance, review montage, exact regeneration, headroom, DC and silent endpoints.")
    else:
        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        for name, data in outputs.items():
            (OUTPUT_DIR / name).write_bytes(data)
        REVIEW_PATH.parent.mkdir(parents=True, exist_ok=True)
        REVIEW_PATH.write_bytes(review)
        print(f"Generated {len(outputs) - 1} WAVs and PROVENANCE.json in {OUTPUT_DIR.relative_to(ROOT)}; review montage in {REVIEW_PATH.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
