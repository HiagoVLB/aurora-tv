#!/usr/bin/env python3
"""Audit Aurora PO coverage and actual gettext/webOS conversion, read-only."""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

try:
    import polib
except ImportError:
    sys.exit("Missing polib: install it in a virtualenv (python -m pip install polib).")


def key(entry):
    return (entry.msgctxt or "", entry.msgid)


def describe(entry):
    return {"context": entry.msgctxt, "msgid": entry.msgid}


def duplicates(entries):
    return [dict(context=k[0], msgid=k[1], count=n)
            for k, n in Counter(key(e) for e in entries).items() if n > 1]


def unique_json(pairs):
    result = {}
    for k, value in pairs:
        if k in result:
            raise ValueError(f"Duplicate JSON key: {k!r}")
        result[k] = value
    return result


def audit(path, template, converter, msgfmt, awk):
    po = polib.pofile(str(path), encoding="utf-8", check_for_duplicates=False)
    entries = [e for e in po if not e.obsolete and e.msgid]
    active = {key(e): e for e in entries}
    expected = {key(e): e for e in template if not e.obsolete and e.msgid}
    missing = [describe(e) for k, e in expected.items() if k not in active]
    untranslated = [describe(e) for e in entries if not e.translated() and not e.fuzzy]
    fuzzy = [describe(e) for e in entries if e.fuzzy]
    unsupported = [describe(e) for e in entries if e.msgctxt or e.msgid_plural]
    same = [describe(e) for e in entries if e.msgstr == e.msgid]
    with tempfile.TemporaryDirectory(prefix="aurora-i18n-") as temporary:
        checked = subprocess.run([msgfmt, "--check", "--check-format", "-o",
                                  str(Path(temporary) / "catalog.mo"), str(path)],
                                 text=True, capture_output=True)
    converted = subprocess.run([awk, "-f", str(converter), str(path)],
                               text=True, capture_output=True)
    conversion_errors = []
    try:
        if converted.returncode:
            raise ValueError(converted.stderr.strip() or "AWK failed")
        actual = json.loads(converted.stdout, object_pairs_hook=unique_json)
        if not isinstance(actual, dict):
            raise ValueError("cstrings must be a JSON object")
        wanted = {e.msgid: e.msgstr for e in entries
                  if not e.fuzzy and not e.msgctxt and not e.msgid_plural and e.msgstr}
        for msgid in sorted(set(wanted) | set(actual)):
            if actual.get(msgid) != wanted.get(msgid):
                conversion_errors.append({"msgid": msgid, "expected": wanted.get(msgid),
                                          "actual": actual.get(msgid)})
    except ValueError as error:
        conversion_errors.append({"error": str(error)})
    repeated = duplicates(entries)
    failed = bool(missing or untranslated or fuzzy or unsupported or repeated
                  or checked.returncode or conversion_errors)
    return {
        "status": "failed" if failed else "passed",
        "entries": len(entries), "template_entries": len(expected),
        "translated_current": sum(k in expected and e.translated() for k, e in active.items()),
        "missing": missing, "untranslated": untranslated, "fuzzy": fuzzy,
        "duplicates": repeated, "unsupported_webos": unsupported,
        "same_as_source_review": same,
        "extra_to_template": [describe(e) for k, e in active.items() if k not in expected],
        "obsolete": sum(e.obsolete for e in po),
        "gettext": {"returncode": checked.returncode, "diagnostics": checked.stderr.strip()},
        "webos_errors": conversion_errors,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path.cwd())
    parser.add_argument("--template", type=Path, help="POT path relative to repo or absolute")
    parser.add_argument("--locale", action="append", help="Repeat to restrict scope")
    parser.add_argument("--output", type=Path, help="Detailed JSON; sources remain unchanged")
    args = parser.parse_args()
    repo = args.repo.resolve()
    template_path = args.template or Path("src/i18n/messages.pot")
    if not template_path.is_absolute():
        template_path = repo / template_path
    converter = repo / "scripts/webos/po2json.awk"
    paths = sorted((repo / "src/i18n").glob("*/messages.po"))
    names = {p.parent.name for p in paths}
    if args.locale:
        unknown = set(args.locale) - names
        if unknown:
            parser.error("Unknown locale(s): " + ", ".join(sorted(unknown)))
        paths = [p for p in paths if p.parent.name in args.locale]
    if not paths or not template_path.is_file() or not converter.is_file():
        parser.error("Aurora PO catalogs, POT or webOS converter not found")
    msgfmt, awk = shutil.which("msgfmt"), shutil.which("gawk") or shutil.which("awk")
    if not msgfmt or not awk:
        parser.exit(2, "Missing msgfmt/gettext or AWK; checks are pending, not passed.\n")
    try:
        template = polib.pofile(str(template_path), encoding="utf-8", check_for_duplicates=False)
        template_duplicates = duplicates([e for e in template if not e.obsolete and e.msgid])
        reports = {}
        for path in paths:
            try:
                reports[path.parent.name] = audit(path, template, converter, msgfmt, awk)
            except (OSError, ValueError) as error:
                reports[path.parent.name] = {"status": "failed", "error": str(error)}
        cmake = (repo / "CMakeLists.txt").read_text(encoding="utf-8")
    except (OSError, ValueError) as error:
        parser.exit(2, f"Unable to audit: {error}\n")
    match = re.search(r"set\s*\(\s*I18N_LOCALES\s+([^)]*)\)", cmake)
    packaged = re.findall(r'[\w]+(?:-[\w]+)*', match.group(1)) if match else None
    failed = bool(template_duplicates or any(r["status"] != "passed" for r in reports.values()))
    result = {
        "status": "failed" if failed else "passed", "repo": str(repo),
        "template": str(template_path), "template_duplicates": template_duplicates,
        "catalogs": sorted(names), "packaged_cmake_literal": packaged,
        "catalogs_not_packaged": sorted(names - set(packaged)) if packaged is not None else None,
        "note": "CMake inventory is a literal scan; verify selector/DEBUG/runtime separately. "
                "No semantic, unflagged printf, source freshness, font or visual validation performed.",
        "locales": reports,
    }
    if args.output:
        output = args.output.resolve()
        if output.suffix != ".json":
            parser.error("--output must be a separate .json report")
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for locale, report in reports.items():
        counts = " ".join(f"{name}={len(report.get(name, []))}"
                          for name in ("missing", "untranslated", "fuzzy", "webos_errors"))
        print(f"{locale}: {report['status']} {counts}")
    print(f"Overall: {result['status']}; {args.output or 'use --output for details'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
