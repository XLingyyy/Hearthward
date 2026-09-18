"""Download the pinned Windows local-inference bundle into this project, never global tools."""
from pathlib import Path
import hashlib
import json
import shutil
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def download(url, destination, expected_hash, expected_size=None):
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        with destination.open("rb") as existing:
            digest = hashlib.sha256()
            for data in iter(lambda: existing.read(8 * 1024 * 1024), b""):
                digest.update(data)
        if digest.hexdigest() == expected_hash:
            return
        raise ValueError(f"Existing file does not match lock: {destination.name}")
    partial = destination.with_suffix(destination.suffix + ".part")
    offset = partial.stat().st_size if partial.exists() else 0
    headers = {"User-Agent": "Hearthward-LocalAI-Setup/1"}
    if offset:
        headers["Range"] = f"bytes={offset}-"
    request = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(request, timeout=120) as response:
        if offset and response.status != 206:
            offset = 0
        digest = hashlib.sha256()
        if offset:
            with partial.open("rb") as existing:
                for data in iter(lambda: existing.read(8 * 1024 * 1024), b""):
                    digest.update(data)
        total, reported = offset, offset
        with partial.open("ab" if offset else "wb") as stream:
            while data := response.read(8 * 1024 * 1024):
                stream.write(data)
                digest.update(data)
                total += len(data)
                if total - reported >= 256 * 1024 * 1024:
                    print(f"{destination.name}: {total // (1024 * 1024)} MiB", flush=True)
                    reported = total
    if (expected_size is not None and total != expected_size) or digest.hexdigest() != expected_hash:
        raise ValueError(f"Downloaded file does not match lock: {destination.name}")
    partial.replace(destination)
    print(f"Verified {destination.name}: {total} bytes", flush=True)


def extract_runtime(archive, target):
    target.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as z:
        for info in z.infolist():
            if info.is_dir():
                continue
            relative = Path(info.filename)
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError("Unsafe runtime archive path")
            # Release archives may contain a build directory; runtime DLLs share one directory.
            if relative.suffix.lower() not in {".exe", ".dll", ".txt"} and not relative.name.startswith("LICENSE"):
                continue
            output = (target / relative.name).resolve()
            if output.parent != target.resolve():
                raise ValueError("Runtime output escaped bundle")
            with z.open(info) as source, output.open("wb") as dest:
                shutil.copyfileobj(source, dest)
    if not (target / "llama-server.exe").is_file():
        raise ValueError("Release is missing llama-server.exe")


def main():
    lock = json.loads((ROOT / "config/local-ai.lock.json").read_text(encoding="utf-8"))
    bundle = ROOT / "Runtime/LocalAI"
    runtime = lock["runtime"]
    for entry in runtime["artifacts"]:
        archive = bundle / ".downloads" / entry["filename"]
        url = f"https://github.com/{runtime['repository']}/releases/download/{runtime['revision']}/{entry['filename']}"
        download(url, archive, entry["sha256"])
        extract_runtime(archive, bundle / "bin" / entry["backend"])
    model = lock["model"]
    download(f"https://huggingface.co/{model['repository']}/resolve/{model['revision']}/{model['filename']}",
             bundle / "models" / model["filename"], model["sha256"], model["size"])
    print("Project-local inference bundle ready. No global installation performed.", flush=True)


if __name__ == "__main__":
    main()
