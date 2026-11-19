"""Prepare the pinned PresentMon dependency for a developer build."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import tempfile
from urllib.request import urlopen


DIRECTORY = Path(__file__).resolve().parents[1] / "vendor" / "PresentMon"
FILENAME = "PresentMon.exe"
URL = "https://github.com/GameTechDev/PresentMon/releases/download/v2.4.1/PresentMon-2.4.1-x64.exe"


def verify(path, expected):
    with path.open("rb") as stream:
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    if digest.hexdigest() != expected:
        raise ValueError(f"SHA-256 mismatch for {path.name}")


def prepare(source):
    sums = {}
    for line in (DIRECTORY / "SHA256SUMS").read_text(encoding="ascii").splitlines():
        digest, name = line.split(maxsplit=1)
        sums[name] = digest
    for name in ("LICENSE.txt", "THIRD_PARTY.txt"):
        verify(DIRECTORY / name, sums[name])
    destination = DIRECTORY / FILENAME
    if destination.exists():
        verify(destination, sums[FILENAME])
        return
    with tempfile.TemporaryDirectory(dir=DIRECTORY) as temporary:
        staged = Path(temporary) / FILENAME
        with (source.open("rb") if source else urlopen(URL, timeout=90)) as stream:
            with staged.open("wb") as output:
                shutil.copyfileobj(stream, output)
        verify(staged, sums[FILENAME])
        os.replace(staged, destination)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, help="use an existing x64 file for an offline build")
    args = parser.parse_args()
    try:
        prepare(args.source)
    except (OSError, ValueError) as error:
        parser.exit(1, f"PresentMon 2.4.1: {error}\n")
    print("PresentMon 2.4.1 and upstream notices verified.")
