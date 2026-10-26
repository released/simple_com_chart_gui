# Native interface language and user guides

Approved scope (2026-10-26): external lang_en.ini / lang_tw.ini, a saved language
selection, Traditional Chinese by default, docs/USER_GUIDE.md as the Traditional
Chinese product entry and a complete docs/USER_GUIDE_EN.md. The current checkout
has no USER_GUIDE.md; create it at the requested path without replacing existing
specialist guides. Other competitive-analysis recommendations remain research only.

Implementation sequence:

1. Add a bounded native UTF-8 INI catalog with stable text identifiers, English
   source fallback and an English/Traditional Chinese choice saved as UI/Language.
   A valid saved choice wins; missing/invalid preference defaults to tw. Missing
   or malformed translations fall back visibly/safely, never block UART operation.
2. Localize native menus, tabs, captions, field labels, static choices and hover
   help at presentation boundaries. Preserve canonical choice values, protocol
   tokens, user input/keys/aliases, raw RX/TX, file formats and numeric exports.
   Do not introduce a global translation hook on received text. Core diagnostics
   and OS-provided error/file-dialog text can retain their original language.
3. Settings > Language saves the next-launch choice without rebuilding windows,
   reconnecting, discarding capture or transmitting. Package both INIs beside the
   GUI EXE from src/i18n; retain standalone English fallback and static MFC/CRT.
4. Write complete matching user guides and link them from README. Keep engineering
   history, acceptance and limitations in SELF_CHECK/HANDOFF/compact evidence.
5. Verify catalog parsing, malformed/missing entries, stored choice/failure cases,
   real native Chinese/English controls and canonical enum/input preservation,
   minimum-size/enlarged-font layout, language-save-with-live-data behavior and
   final formal delivery. Select affected tests only; no UART hardware or Python.
6. Final safe clean and retire exact owned compiler/test/baseline files, preserving
   delivery INIs/EXE, private settings, Git and unknown data. Persist the concise
   cleanup receipt before retiring its full temporary registry. Reuse the existing
   explicit 45 MB final Git-retention decision; no temporary-size approval needed.

Entry delivery: 1.0.1.029 GUI 2,272,256 bytes, SHA256
99286CCCFD7C8A0C7C9D8800EB31E6B1B2E0CADE3F8EFF0342BC797AAF166803.
Personal INI SHA256:
1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
Preserve all existing uncommitted .028/.029 work. No commit/push is implied.

Implementation verified in 1.0.1.030: all six steps are tracked by the compact
receipt in verification/latest-validation.json. Both guides have matching 14
sections; default TW and explicit EN formal startup passed. Language applies on
next launch; technical diagnostics can remain English. The source comparison
found no missing entry files. Final safe cleanup and retained-byte accounting
belong to that receipt, not an additional permanent build/verify archive.
