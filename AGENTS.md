# Liberty Music Software - mandatory UI text policy

- All source files are strict UTF-8, NFC for human-facing literals.
- C++17 UI strings containing accents, typographic punctuation or symbols MUST use
  LIBERTY_UTF8(u8"..."), defined in Source/LibertyUtf8.h. Use juce::String
  arrays and return values for translated UI text. Never rely on the
  implicit const char* constructor for non-ASCII text.
- Do not remove French accents as a workaround. French is the default.
- Keep /utf-8 on every MSVC target that compiles localized strings.
- Run python tools/check_unicode.py before building and the LibertyUnicode
  CTest runtime test before uploading binaries. A failure blocks release.
- Check all five pages and every advanced panel, dropdown, tooltip,
  context menu, saved preset name and both languages on the compiled
  plugin. A successful compile alone is not proof of visual quality.
- Preserve the 0.3.0 native layout, DSP, preset bank and MIDI learn.
- Reuse this policy and test infrastructure when creating or editing any
  Liberty Music Software instrument or effect. For existing projects,
  port the guard and adapt its include path before changing the UI.
