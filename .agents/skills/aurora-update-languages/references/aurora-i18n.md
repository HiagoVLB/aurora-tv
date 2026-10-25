# Localization map and commands

## Behavior-defining files

| Contract | Files |
| --- | --- |
| Catalogs/source | `src/i18n/*/messages.po`, `src/i18n/messages.pot`, `locstr`/`translatable` uses in `src/` |
| Packaging list | `I18N_LOCALES` in `CMakeLists.txt` |
| Selector/normalization | `src/app/platform/common/i18n_common.c` |
| Desktop gettext | `cmake/MoonlightI18n.cmake`, `src/app/platform/common/{app_common,i18n_gettext}.c` |
| webOS JSON/runtime | `cmake/PackageWebOS.cmake`, `scripts/webos/po2json.awk`, `src/app/platform/webos/i18n_webos.c` |
| Fonts/fallback | `src/app/util/font.c`, `FONT_FAMILY`/`FONT_FAMILY_FALLBACK`, Fontconfig |
| Real UI review | `tools/ui-sim/{README.md,main.c,run.sh,run.py,Dockerfile}` |

Initial inventory (2026-10-18; historical): 16 catalogs, only 13 in
`I18N_LOCALES`; `ko` selectable but not packaged; `uk`/`zh-TW` catalog-only;
`ja` DEBUG-only selector. The 2026-10-19 review added Korean to packaging,
bringing the packaged set to 14. Reconfirm current lists. English is source,
without a separate PO. Updating existing catalogs does not enable new languages.

webOS uses flat JSON rather than full gettext semantics. Current AWK joins
lines/ignores empty translations but does not filter fuzzy or implement contexts/
plurals. Removing fuzzy without review hides desktop/TV divergence. Auditor
flags these cases and compares effective JSON against expected PO content.

## Prepare tools

Use gettext (`xgettext`, `msgmerge`, `msgfmt`), AWK, Python with `polib`.
Reuse existing environment or create a local ignored venv:

```sh
python3 -m venv .cache/i18n/venv
.cache/i18n/venv/bin/python -m pip install polib
.cache/i18n/venv/bin/python .agents/skills/aurora-update-languages/scripts/audit_catalogs.py \
  --repo . --output .cache/i18n/before.json
```

Without Mac gettext, use prepared Linux environment such as
`aurora-ui-sim:local` if available. Missing `msgfmt` means pending check and exit 2,
not pass. Do not install into global Python or use `sudo` as first choice.
Auditor also requires AWK/`polib` in that environment.

## Extract and merge

From Aurora root with Python/gettext in the same environment, generate temporary POT:

```sh
mkdir -p .cache/i18n
python3 - <<'PY'
from pathlib import Path
import subprocess
files = sorted(str(p) for p in Path('src').rglob('*') if p.suffix in {'.c', '.h'})
subprocess.run(['xgettext', '--from-code=UTF-8', '--keyword=locstr',
                '--keyword=translatable', '--add-comments',
                '-o', '.cache/i18n/current.pot', *files], check=True)
PY
```

Compare tracked POT, preserve legitimate local entries, update source file after
inspection. Merge each in-scope PO separately:

```sh
msgmerge --update --backup=none src/i18n/pt-BR/messages.po src/i18n/messages.pot
msgfmt --check --check-format -o .cache/i18n/pt_BR.mo src/i18n/pt-BR/messages.po
```

Do not run only pt-BR for an all-language request. Use
`--template .cache/i18n/current.pot` to audit temporary extraction. Compare before/
after against the same current POT for consistent coverage. Inspect stderr even
on exit zero: headers may cause warnings.

## Prepare localized visual checks

Desktop domain: `moonlight-tv`. MO paths:
`<mo-root>/<underscore_locale>/LC_MESSAGES/moonlight-tv.mo`.
Set `TEXTDOMAINDIR` to `<mo-root>` and ensure UTF-8 locale exists on Linux
(`locale -a`). Standalone `aurora-ui-sim` target may not build `moonlight-i18n`;
check and explicitly build/generate test MOs.

Current wrapper supports `AURORA_UI_SIM_LANGUAGE`, prepares locales/CJK fonts,
and loads compiled MO files; confirm in the checked-out README/scripts. At skill
creation, this support was absent, preference fixed to `en`, and desktop startup
used environment `setlocale("")`. Settings controllers also call `i18n_setlocale`
with preference. Align environment/preference to avoid language changes during
navigation. Changing preferences after widget creation may require UI rebuild.

Reuse abstract actions (`settings`, `help`, `add_host`), translating `click_text`
and `expect` through each actual catalog. Assert a unique translated string;
English-identical acronyms do not prove loading. Inspect PNGs/logs in
`.cache/ui-sim-run-*/artifacts/`.

For ko/ja/zh and Cyrillic, check actually rendered glyphs. Additional Docker
fonts validate simulator coverage, not LG fonts. Maintain language → screen →
resolution → capture → result/pending table.
