#!/usr/bin/env python3
"""Capture local Git evidence for Aurora/reference comparisons; no network access."""

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys


def git(root, *args):
    return subprocess.check_output(
        ["git", "--no-optional-locks", "-C", str(root), *args],
        stderr=subprocess.PIPE,
    )


def digest(data):
    return hashlib.sha256(data).hexdigest()


def untracked_entry(root, name):
    path = root / os.fsdecode(name)
    if path.is_symlink():
        return {"path": os.fsdecode(name), "type": "symlink",
                "sha256": digest(os.fsencode(os.readlink(path)))}
    checksum = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            checksum.update(chunk)
    return {"path": os.fsdecode(name), "type": "file",
            "sha256": checksum.hexdigest()}


def repository(root):
    root = Path(os.fsdecode(git(root, "rev-parse", "--show-toplevel")).strip())
    head = git(root, "rev-parse", "HEAD").decode().strip()
    # NUL form preserves renames and filenames containing spaces/newlines.
    status = git(root, "status", "--porcelain=v1", "-z", "--untracked-files=all")
    untracked = git(root, "ls-files", "--others", "--exclude-standard", "-z")
    submodules = []
    for record in git(root, "ls-files", "--stage", "-z").split(b"\0"):
        if not record:
            continue
        metadata, name = record.split(b"\t", 1)
        mode, pinned_head, stage = metadata.split()
        if mode != b"160000" or stage != b"0":
            continue
        path = root / os.fsdecode(name)
        entry = {"path": os.fsdecode(name), "pinned_head": pinned_head.decode()}
        entry["initialized"] = (path / ".git").exists()
        if entry["initialized"]:
            entry["state"] = repository(path)
        submodules.append(entry)
    state = {
        "head": head,
        "dirty": bool(status),
        "status_porcelain_z": status.decode("utf-8", "surrogateescape").split("\0")[:-1],
        "staged_diff_sha256": digest(git(root, "diff", "--cached", "--binary",
                                         "--no-ext-diff", "--no-textconv")),
        "unstaged_diff_sha256": digest(git(root, "diff", "--binary",
                                           "--no-ext-diff", "--no-textconv")),
        "untracked_files": [untracked_entry(root, name)
                            for name in sorted(untracked.split(b"\0")) if name],
        "submodules": submodules,
    }
    # Paths/time are excluded: equivalent checkouts yield the same identifier.
    state["state_sha256"] = digest(json.dumps(state, sort_keys=True).encode())
    return state


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--aurora-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--reference", action="append", default=[], metavar="NAME=PATH")
    parser.add_argument("--output", type=Path, help="Prefer an ignored .cache/comparisons path")
    args = parser.parse_args()
    roots = {"aurora": args.aurora_root}
    for value in args.reference:
        name, separator, path = value.partition("=")
        if not separator or not name or not path or name in roots:
            parser.error("Each reference must be a unique NAME=PATH (aurora is reserved)")
        roots[name] = Path(path).expanduser()
    try:
        result = {
            "schema_version": 1,
            "captured_at_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
            "remote_freshness_checked": False,
            "repositories": {name: {"root": str(path.resolve()), "state": repository(path)}
                             for name, path in roots.items()},
        }
        rendered = json.dumps(result, ensure_ascii=True, indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(rendered, encoding="utf-8")
            print(args.output)
        else:
            print(rendered, end="")
    except (OSError, subprocess.CalledProcessError) as error:
        detail = error.stderr.decode(errors="replace").strip() if isinstance(error, subprocess.CalledProcessError) else str(error)
        print(f"Snapshot failed: {detail}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
