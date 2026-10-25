---
name: aurora-update-languages
description: "Update and review Aurora's existing webOS languages: synchronize gettext, translate naturally in context, preserve placeholders, and check cstrings resources, fonts, truncation, and navigation in the real UI. Use to update translations, complete languages, review localization, or fix translated text that does not fit or display correctly."
---

# Update Aurora languages

Locate Aurora root and read [AGENTS.md](../../../AGENTS.md). Complete translation,
technical validation, and visual inspection of affected languages. For skill
creation/maintenance only, test fixtures/temporary copies without changing actual
catalogs as a side effect.

## Define scope and context

1. Check `git status --short`; preserve local changes, translations, credits.
2. Inventory **all** `src/i18n/*/messages.po`, `I18N_LOCALES` in `CMakeLists.txt`,
   and registration in `src/app/platform/common/i18n_common.c`. Distinguish
   existing catalogs, selectable languages, DEBUG-only languages, packaged
   resources. Do not limit a general update to pt-BR or assume lists match.
   Include all existing catalogs for general updates; visually inspect selectable/
   affected languages and explain inaccessible ones.
3. Respect explicitly requested subsets. Do not incidentally add languages or
   enable inactive catalogs. Investigate/fix missing resources for an already
   selectable language when needed for it to work.
4. Read each new/changed string in consuming code: screen, action, state,
   available space, formatting, nearby terminology. Use current English as
   meaning source; do not translate all languages through pt-BR.
5. Read [references/aurora-i18n.md](references/aurora-i18n.md) before synchronizing/
   testing. Reconfirm mutable details against current checkout.

## Synchronize catalogs

- Extract current `src/` strings with CMake's `locstr`/`translatable` keywords.
  Generate temporary POT first; inspect entries, duplicates, local changes before
  replacing `src/i18n/messages.pot`. Include concatenated/multiline strings and `.h`.
- Do not rely only on existing POT: it may contain removed text or miss new screens.
  Do not rename msgids for easier translation; change source only for actual copy
  problems or visible unlocalized text.
- Merge every in-scope catalog using `msgmerge --update --backup=none` and the
  checked POT. Current `i18n-merge-po` target only updates Japanese; do not treat
  it as covering all languages.
- Review empty/fuzzy entries, inherited translations with changed meanings,
  source-identical strings. Remove `fuzzy` **only after reviewing meaning and
  translation**, never in bulk to inflate coverage. Retain natural source-identical
  terms such as HDR, FPS, proper names.
- Preserve UTF-8, useful comments, metadata, regional variants. Check `Language`
  and `Plural-Forms`; do not invent translator names/emails.

## Translate as each language's interface

- Adapt vocabulary, word order, address, capitalization, punctuation naturally
  for the locale. Prefer short clear actions; avoid literal word-by-word translation
  or treating regional variants as interchangeable.
- Keep consistent per-language glossary across settings, help, errors, menus,
  overlay. Distinguish TV remote, gamepad, pointer/virtual mouse. Preserve
  Disconnect (leave stream, app remains on host), End session (terminate on host),
  Resume, Remote Input, Remote Monitor distinctions.
- Preserve product/protocol/key/chord names when identifiers; explain function
  locally when useful. Do not invent capabilities or alter shortcuts to fit layout.
- Preserve printf arguments (`%s`, `%d`, `%u`, `%zu`, precision, `%%`, `*`), types,
  count, value association. Reorder positional arguments only if consumers/platforms
  support it and validation confirms. Check actual printf uses for `c-format`;
  `msgfmt` does not cover strings without that flag.
- Preserve meaningful breaks, `\n`-separated dropdown items, index/option order,
  LVGL symbols, markup, URLs, technical units. Adapt surrounding text without
  changing token contracts.
- Rephrase long translations without losing information. If still too long,
  minimally adjust affected component wrapping/height/scroll and verify other
  languages. Do not generically remove accents, shrink all fonts, or omit warnings.
- Review each language in a second pass against context/glossary. Record specific
  uncertainties; filled msgstr does not establish completed linguistic review.

## Validate both resource paths

Run bundled auditor before/after with Python `polib` and gettext available.
Follow reference setup. From Aurora root:

```sh
python3 .agents/skills/aurora-update-languages/scripts/audit_catalogs.py \
  --repo . --output .cache/i18n/after.json
```

Use `--locale pt-BR --locale fr` only for restricted scope. Auditor is read-only;
reports coverage, fuzzy, duplicates, gettext failures, English equality, differences
between PO and JSON from the real AWK. Fix in-scope failures and rerun. It does
not replace current POT extraction, semantic review, printf-flag checks, visual QA.

Also check catalog → installed path → runtime locale. Verify, for example,
`resources/pt/BR/cstrings.json`, without silently changing `es-ES` to `es`.
Do not introduce gettext contexts/plurals without implementing support in flat
JSON conversion and webOS consumer.

## Check actual display

1. Read `tools/ui-sim/main.c`, `run.sh`, `run.py`, README. Check startup locale
   order and settings reapplication: a preference literal alone does not determine
   initial desktop locale (`setlocale("")` uses environment). Prepare locale/MO,
   set `TEXTDOMAINDIR`, and prove through logs/tree and unique translated text
   that language loaded and remains correct while navigating. English default
   runs do not validate translations; changing only `LANG` may be insufficient.
2. Use existing simulator language support (`AURORA_UI_SIM_LANGUAGE`); if support
   is missing in another checkout, implement minimal test support while preserving
   isolated state/product defaults. Pass through Docker and prepare Linux locale;
   do not replace English with another hardcoded language. Check fonts/fallback.
3. Create per-language scenarios with translated expectations. Open launcher,
   Stream/Input/Host/Other settings, language dropdown, affected help/shortcuts,
   errors, confirmations. Exercise overlay in a safe fixture when possible;
   explicitly record inaccessible screens. Older checkouts may call Other
   Experimental; verify current registered panes.
4. Capture and **open/inspect** real-UI PNGs alongside element trees at 1920×1080
   and 1280×720, or justified target-equivalent resolutions. Check truncation,
   information-hiding ellipsis, overlap, wrapping, accents, CJK/Cyrillic, missing
   glyphs, contrast, alignment. String length, full tree text, passing `report.json`
   do not establish correct rendering.
5. Navigate with D-pad/controller, scroll long messages, close with Back/Escape;
   verify focus return, option selection, reopening. Repeat affected captures
   after truncation/layout fixes.
6. Distinguish Linux fonts/locale from LG fonts/bundle. Validate webOS resources
   locally; report actual C2 testing separately. If Docker/locale/fonts/TV are
   unavailable, complete possible work and report visual/physical pending checks
   without marking them passed.

## Deliver evidence

Check diff, links, translations; update relevant `docs/CURRENT_STATE.md` area
with languages, before/after coverage, checks, capture paths, pending work.
Report adaptations, languages actually viewed in UI, validations limited to catalogs.

Do not generate IPK, bump version, install on TV, publish, commit/push as skill
side effects. If IPK/version is requested, run corresponding skills after language
review.
