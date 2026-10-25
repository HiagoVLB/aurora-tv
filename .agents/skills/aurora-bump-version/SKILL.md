---
name: aurora-bump-version
description: "Bump Aurora's central CMakeLists.txt version and check version derivation in appinfo, APP_VERSION, and the ELF marker. Use for requested version increases or changes; defaults to patch, without automatically building an IPK, creating tags, or publishing releases."
---

# Bump Aurora version

Locate Aurora root and read [AGENTS.md](../../../AGENTS.md). Treat
`MOONLIGHT_VERSION` in `CMakeLists.txt` as the only editable product version.
`deploy/webos/appinfo.json` uses `@PROJECT_VERSION@`, UI uses `APP_VERSION`,
and executable `aurora-v` marker must concatenate `APP_VERSION`.

## Apply the requested increment

From Aurora root, check and apply:

```sh
python3 scripts/dev/aurora_version.py --check
python3 scripts/dev/aurora_version.py --part patch --apply
python3 scripts/dev/aurora_version.py --check
```

Default to **patch** when only a version bump is requested. Honor explicit
minor/major or exact versions:

```sh
python3 scripts/dev/aurora_version.py --part minor --apply
python3 scripts/dev/aurora_version.py --part major --apply
python3 scripts/dev/aurora_version.py --set 1.4.0 --apply
```

These are alternatives, not sequential increments. Omit `--apply` for a dry run.
Use numeric `X.Y.Z` without `v`, suffixes, or leading zeroes. Reject same-version
and downgrade requests. Do not bump twice when repeating validation: `--check`
does not modify files.

If checking finds a broken contract (duplicate version, literal appinfo version,
stale ELF marker), inspect/fix that contract within scope. Do not globally replace
versions: historical reports, submodules, resource tools have independent versions.

## Record and combine with builds

Inspect the diff: normal operation changes only `MOONLIGHT_VERSION`. Update
current declared version and change record in
[CURRENT_STATE.md](../../../docs/CURRENT_STATE.md) without rewriting historical
package identities. Report old/new versions.

Existing IPKs, including `aurora.ipk`, retain their old version until rebuilt.
If an installer is requested, run
[aurora-build-ipk](../aurora-build-ipk/SKILL.md) afterward; it replaces the fixed
file only after validating the new version. Do not edit an already built IPK.

Do not create commits/tags/releases, push, or install on TV as bump side effects.
For skill creation/maintenance only, validate in a temporary copy without
bumping the actual project version.
