#!/usr/bin/env python3
"""Stage reusable assets and arm64 native libraries from a prior release APK."""

from __future__ import annotations

import argparse
import os
from pathlib import Path, PurePosixPath
import shutil
import tempfile
import zipfile


DEFAULT_EXPECTED_VERSION_NAME = "9.2.3-ap1.4.2-p7"
REQUIRED_ENTRIES = {
    "assets/soh.o2r",
    "lib/arm64-v8a/libSDL2.so",
    "lib/arm64-v8a/libsoh.so",
}


class InvalidApk(ValueError):
    """Raised when an APK is unsafe or lacks required release inputs."""


def _safe_archive_name(name: str) -> PurePosixPath:
    if not name or "\\" in name:
        raise InvalidApk(f"unsafe ZIP entry name: {name!r}")
    path = PurePosixPath(name)
    if not path.parts or path.is_absolute() or ".." in path.parts or path.parts[0] in ("", "."):
        raise InvalidApk(f"unsafe ZIP entry name: {name!r}")
    return path


def _selected_destination(name: str, android_dir: Path) -> Path | None:
    if name.startswith("assets/") and not name.endswith("/"):
        relative = PurePosixPath(name).relative_to("assets")
        return android_dir / "app/src/main/assets" / Path(*relative.parts)
    if name.startswith("lib/arm64-v8a/") and name.endswith(".so"):
        relative = PurePosixPath(name).relative_to("lib/arm64-v8a")
        if len(relative.parts) != 1:
            raise InvalidApk(f"native library must be directly under lib/arm64-v8a: {name}")
        return android_dir / "app/src/main/jniLibs/arm64-v8a" / relative.name
    return None


def prepare(base_apk: Path, expected_version_name: str, android_dir: Path) -> list[Path]:
    if not base_apk.is_file():
        raise InvalidApk(f"base APK does not exist: {base_apk}")
    if not expected_version_name or expected_version_name not in base_apk.name:
        raise InvalidApk(
            f"base APK filename {base_apk.name!r} does not contain expected version "
            f"{expected_version_name!r}"
        )
    if not (android_dir / "app/src/main").is_dir():
        raise InvalidApk(f"invalid Android project directory: {android_dir}")

    selected: list[tuple[zipfile.ZipInfo, Path]] = []
    seen: set[str] = set()
    present: set[str] = set()
    try:
        with zipfile.ZipFile(base_apk) as archive:
            for info in archive.infolist():
                name = str(_safe_archive_name(info.filename))
                if name in seen:
                    raise InvalidApk(f"duplicate ZIP entry: {name}")
                seen.add(name)
                if info.is_dir():
                    continue
                present.add(name)
                destination = _selected_destination(name, android_dir)
                if destination is not None:
                    selected.append((info, destination))

            missing = sorted(REQUIRED_ENTRIES - present)
            if missing:
                raise InvalidApk("base APK is missing required entries: " + ", ".join(missing))

            staged: list[tuple[Path, Path]] = []
            with tempfile.TemporaryDirectory(prefix="prebuilt-release-", dir=android_dir) as temp_name:
                temp_dir = Path(temp_name)
                for index, (info, destination) in enumerate(selected):
                    staged_file = temp_dir / str(index)
                    with archive.open(info) as source, staged_file.open("wb") as output:
                        shutil.copyfileobj(source, output)
                    staged.append((staged_file, destination))

                for staged_file, destination in staged:
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    temporary = destination.with_name(destination.name + ".prebuilt.tmp")
                    shutil.copyfile(staged_file, temporary)
                    os.replace(temporary, destination)
    except zipfile.BadZipFile as error:
        raise InvalidApk(f"invalid APK ZIP: {error}") from error

    return [destination for _, destination in selected]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base_apk", type=Path)
    parser.add_argument("android_dir", type=Path)
    parser.add_argument(
        "--expected-version-name",
        default=DEFAULT_EXPECTED_VERSION_NAME,
        help="prior APK version expected in the artifact filename",
    )
    args = parser.parse_args()

    try:
        written = prepare(args.base_apk, args.expected_version_name, args.android_dir)
    except (InvalidApk, OSError) as error:
        parser.error(str(error))

    print(
        f"Prepared {len(written)} files from {args.expected_version_name}. "
        "Caller must verify the APK checksum and identity before this step."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
