#!/usr/bin/env python3
"""Read or increment Aurora's single project version, without tagging or building."""

import argparse
from pathlib import Path
import re
import sys

VERSION = r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)"
SETTING = re.compile(r'^set\(MOONLIGHT_VERSION "(' + VERSION + r')"\)$', re.MULTILINE)


def read_version(root):
    text = (root / "CMakeLists.txt").read_text()
    matches = list(SETTING.finditer(text))
    if len(matches) != 1:
        raise ValueError('Expected exactly one set(MOONLIGHT_VERSION "X.Y.Z")')
    return matches[0].group(1)


def check_contract(root):
    version = read_version(root)
    manifest = (root / "deploy/webos/appinfo.json").read_text()
    import json
    if json.loads(manifest).get("version") != "@PROJECT_VERSION@":
        raise ValueError("appinfo.json must derive its version from @PROJECT_VERSION@")
    worker = (root / "src/app/stream/session_worker.c").read_text()
    if not re.search(r'aurora_build_tag\[\].*=\s*"aurora-v"\s+APP_VERSION\s*;', worker):
        raise ValueError("ELF build tag must derive from APP_VERSION")
    cmake = (root / "CMakeLists.txt").read_text()
    if 'APP_VERSION="${PROJECT_VERSION}"' not in cmake:
        raise ValueError("APP_VERSION must derive from PROJECT_VERSION")
    return version


def next_version(current, part="patch", explicit=None):
    old = tuple(map(int, current.split(".")))
    if explicit is not None:
        if not re.fullmatch(VERSION, explicit):
            raise ValueError("Use a numeric X.Y.Z version without v, suffixes or leading zeros")
        new = tuple(map(int, explicit.split(".")))
    else:
        index = {"major": 0, "minor": 1, "patch": 2}[part]
        new = tuple(old[i] if i < index else old[i] + 1 if i == index else 0
                    for i in range(3))
    if new <= old:
        raise ValueError("The new version must be greater than the current version")
    return ".".join(map(str, new))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    operation = parser.add_mutually_exclusive_group()
    operation.add_argument("--part", choices=("patch", "minor", "major"), default=None)
    operation.add_argument("--set", dest="explicit", metavar="X.Y.Z")
    operation.add_argument("--check", action="store_true")
    parser.add_argument("--apply", action="store_true", help="Write the change; otherwise preview only")
    args = parser.parse_args()
    if args.check and args.apply:
        parser.error("--check does not modify files; omit --apply")
    try:
        current = check_contract(args.root)
        if args.check:
            print(f"Version consistent: {current}")
            return 0
        target = next_version(current, args.part or "patch", args.explicit)
        if args.apply:
            path = args.root / "CMakeLists.txt"
            text = path.read_text()
            # Only the central version changes; templates and historical documents stay intact.
            updated = SETTING.sub(lambda match: f'set(MOONLIGHT_VERSION "{target}")', text)
            path.write_text(updated)
            check_contract(args.root)
        print(f"{current} -> {target} ({'applied' if args.apply else 'preview; use --apply to write'})")
        return 0
    except (OSError, ValueError) as error:
        print(f"Version error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
