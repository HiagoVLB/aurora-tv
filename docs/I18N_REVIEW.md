# Aurora language review — 2026-10-19

All 16 existing catalogs were synchronized with the code and reviewed in context.
The initial baseline had 210 extracted messages; localizing the
`%d * %d (Native)` indicator brought the final count to **211**. Strings removed
from code remain obsolete entries and do not replace current translations.

## Coverage

Counts of current, filled, non-fuzzy messages before and after the review.
Technical names, button identifiers, and acronyms may match English.

| Language | Before | After | Language | Before | After |
| --- | ---: | ---: | --- | ---: | ---: |
| cs | 67 | 211 | de | 46 | 211 |
| es-ES | 39 | 211 | fr | 77 | 211 |
| it | 48 | 211 | ja | 49 | 211 |
| ko | 0 | 211 | nl | 67 | 211 |
| pl | 39 | 211 | pt-BR | 99 | 211 |
| ro | 37 | 211 | ru | 54 | 211 |
| tr | 39 | 211 | uk | 0 | 211 |
| zh-CN | 72 | 211 | zh-TW | 0 | 211 |

Adaptation uses English source strings and consistent per-language terminology
across help, menus, and settings. Chinese variants were adapted separately.
Disconnect leaves the application running on the host; End session requests
termination after confirmation. Translations preserve that distinction. Key/chord
names, placeholders, and technical units preserve consumer contracts.

Korean was already selectable but not packaged: it now joins the 14 catalogs in
`I18N_LOCALES`. `uk` and `zh-TW` were updated and exercised in the simulator
without enabling them in the product. Japanese remains conditional on DEBUG.
English remains the source and fallback language.

## Validation performed

- Skill auditor: all 16 catalogs passed, with no missing, empty, or fuzzy current
  messages; `msgfmt --check --check-format` passed, with no duplicates,
  incompatible contexts/plurals, or differences from the real webOS AWK output.
- Each extracted printf placeholder has `c-format`. Argument types, counts,
  and order were preserved.
- Real SDL/LVGL UI, dummy backends, offline runtime: **80 steps per language**,
  covering help, scrolling, settings, resolution dropdown, Input/Host/Experimental
  panes, cancelled reset/Add PC/exit, and Escape return. Every scenario uses
  1920×1080 and 1280×720 and exits normally: **1,280 steps passed**.
- After localizing native resolution, **8 additional steps per language** checked
  the translated dropdown value at 720p and navigation/exit: **128 steps passed**.
- Captures from each language were opened to inspect wrapping, overlap, accents,
  Cyrillic, and CJK. Fontconfig coverage for primary/fallback fonts found no
  missing translated characters in the Linux environment.
- Eight `tests/scripts/test_release_helpers.py` tests, Bash wrapper syntax, and
  `git diff --check` passed.

Tests exposed and prompted fixes for the compact toolbar and Back state after
a dropdown. The truncation case also confirmed that `LV_LABEL_LONG_DOT` requires
copied text: it writes dots into the buffer, which is invalid for static/read-only
gettext strings.

## Reproduction and local artifacts

From the Aurora root with Docker available:

```sh
AURORA_UI_SIM_LANGUAGE=pt-BR ./tools/ui-sim/run.sh tools/ui-sim/localization.json
```

Replace `pt-BR` with the desired catalog code. The runner translates expectations
using the real MO; the log records `UI locale` and the localized language name.

Review artifacts are in Git-ignored `.cache/i18n/`:

- `before.json`, `after.json`: catalog audits;
- `visual-complete/matrix.json`: per-language results/paths, reports, captures,
  UI trees, and logs;
- `native-check/matrix.json`: native-resolution indicator validation;
- `contacts/`: capture sheets used for visual inspection;
- `font-coverage.json`: Linux font coverage.

## Limits

Tests ran neither LG firmware nor a real host session. They do not establish
C2 font availability, `ResBundleC` loading, the webOS-specific language selector,
physical shortcut delivery, PC pairing, actual host errors, or streaming overlay
behavior. Help and string contracts were reviewed in code; cases without an
accessible fixture still require TV validation. No IPK, installation, or version
change was performed.
