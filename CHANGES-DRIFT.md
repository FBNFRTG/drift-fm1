# DRIFT changelog

## 0.9 — Sequence variations and reorganised effects

- DIR: ROOT plus nine movement families with seven ranges each (UP, DN, UD, DU, BOUNCE, OUT, IN, PEDAL, WALK). Old direction values retain their meaning.
- INTERVAL: fixed scale intervals plus 3rd/4th, 3rd/5th, 4th/5th, 2nd/3rd, TRIAD (stacked thirds with octave return), 5th/4th, octave/3rd and 3rd/4th/5th. These are melodic steps in the selected scale, not simultaneous chords. New sequences wrap within the selected note range.
- CHAOS: 0 keeps the ordered pattern; 1–999 select independently seeded permutations. The number selects a variation, not an intensity. Identical pitches cannot become different through shuffling alone.
- FX 1 DELAY: Delay %, Time, Feedback, Type.
- FX 2 REVERB: Reverb %, Size, Type, Tone.
- FX 3 MISC: Chorus, Damp, Motion, Level.
- Delay Time is ordered from short to long, with straight, dotted and triplet values up to 4 bars. Durations are now realised rather than clipped to the buffer. Long delays reduce the delay-line sample rate and therefore sound darker. At 40 BPM, 4 bars = 24 seconds per delay interval / reverse segment.
- Tone now shapes the reverb input; Damp controls the reverb tail. The former stored delay Tone value becomes Reverb Tone. Delay feedback colour is fixed at its former default.
- Patch and sample addresses and 192-byte patch record size are unchanged. CHAOS values above 127 use previously unused record bytes. Old patches load, but nonzero Chaos patterns and FX colour can sound different. Downgrading cannot reproduce the new options.
- FM-1 identity stays FM-1_983; firmware, WASM and installer version 0.9.

# 0.8 — 2026-10-23

Dedicated DRIFT identity FM-1_983 in the package, running firmware and update loader. Release builds retain this identity. Self-hosted project website, complete source downloads, English manual, release manifest and checksums. Synthesis, patch layout and sample storage unchanged from 0.7.

# 0.7

User-facing presets renamed to patches. Same-name saves ask before overwriting (No by default); HOME on the patch list asks to delete, KNOB 1 toggles Yes/No, HOME confirms. User slots are reused; factory deletion is a persistent hide marker, and factory overwrite makes a user copy. Existing flash sectors and record layout preserved; old patches migrate new FX defaults only.

Third FX page: Classic/Ping Pong/Reverse delay; Room/Spring/Hall/Space/Shimmer/Submarine reverb; delay tone and reverb motion. Genuine reverse grains, stereo alternating echoes, octave-up/down pitch shifting and distinct ambient hall voicings. Extra DSP buffers remain within the FM-1 pool budget.

# 0.6.1

Align OCT with preset name on the footer baseline.

# 0.6

Added AIR & SALT as factory preset 21, inspired by spectral/envelope analysis of the user-provided recording. Existing preset IDs and sample layout unchanged.

# 0.5

Octave LEDs with three brightness levels and on-screen OCT label; assignable LFO target (volume, delay send, oscillator 2 pitch/detune) and independent filter depth; vertically centered preset labels.

# 0.4

Buffered display with changed-strip updates; SCL page 2 scale LEDs and key gating; 80 BPM default; GLO page 2 independent oscillator with OFF as first WAVE; oscillator 1 LEVEL; three dual-oscillator presets. Existing sample layout and saved preset records preserved.

# DRIFT changes against Felucca

Base: https://github.com/hugelton/Felucca
Commit: 5738617968a22b5d4c0d3470cfcc95559052f446 (v1.6.1)
Engines base: https://github.com/hugelton/Felucca-Engines
Commit: c46e0086f69bdc26df1cd144dca9e683cc4531dd

DRIFT additions and modifications, 2026-10-20:

- `drift_seq.c`: allocation-free single-part generative note scheduler with bounded
  scale pool, fixed-point event phases, per-event chance and four mutation controls.
- `drift_ui.c`: monochrome screen, factory and user list, duplicated favorites at
  the top, REC favorite toggle, safe new-slot SAVE AS using upstream name entry.
- `seq.c`: replace chain/step playback dispatch with DRIFT scheduler, route incoming
  MIDI to part zero, reset generator at transport start.
- `core.h`, `params.c`: append sixteen parameters before engine parameters; retain
  the extensible preset format and 16-byte track alignment. Ten generator fields
  and six saved space/tempo fields.
- `ui.c`, `ui_input.c`, `ui_draw.c`: DRIFT entrypoints and splash; root and scale
  become part of the saved sound.
- `main.c`: initialize the single-part ambient instrument; mute unused legacy parts.
- `project.c`: disable session autosave and restoration for this reduced interface.
  Existing Felucca autosave sectors are left alone.
- `firmware/engines/src/eng_analog.c`: replace the factory bank with sixteen ambient
  patches. Synthesis code unchanged; MIT license retained.
- `web/emu/felucca_web.c`: compile the new UI into the existing real-firmware emulator.
- `web/emu/index.html`: monochrome DRIFT presentation, quick controls, instructions,
  separate browser storage namespace and a guard against an unsupported sample rate.
- Tests: `tests/drift_test.c`, `web/emu/drift_test.mjs`.

Legacy engine code and firmware storage structures remain in the source for build
and format compatibility, although the DRIFT interface offers one analog part only.
The former Felucca UI remains as unreachable source behind DRIFT entrypoints and is
largely removed by compiler dead-code elimination. This is intentionally a small,
reviewable fork of proven hardware/audio infrastructure, not an independently
rewritten board support package.

The upstream build scripts and artifact filenames remain unchanged. The delivered
firmware package is renamed for distribution. No hardware validation has been done.

All original notices are preserved. The combined work is GPL-3.0-only; dependencies
retain their respective MIT, Apache-2.0 and OFL licenses.

## 0.2 — 2026-10-21

- Deterministic Euclidean 16th-grid generation. All note, velocity, timing and
  probability decisions are frozen until a pattern-setting change. No loop-end mutation.
- Loop length is 1–16 bars of 4/4, factory default 4 bars. All pattern parameters
  invalidate the cached pattern; synth parameters do not.
- Four horizontal parameter columns, page counter, piano roll on both SEQ pages,
  ADSR curve and vertical bars for both FX pages. GLO now opens oscillators.
- Six 40-KiB sample slots at 0xA0000..0xDBFFF, new DMP2 magic. Old sample content
  is not migrated. SLICE disabled; user-preset sectors remain outside this region.
- Analog WAVE adds SAMPLE 1–6, IMA decoding, full resonant SVF, envelope and effects.
  Guard voice state when switching wave types; fix sample slot indexing for slots 5/6.
- New minimal sample uploader with explicit device/port selection and firmware check,
  root note and loop selection, resampling/downmixing, bounded encoding, acknowledged
  block writes and CRC commit. Optional browser-local slots for the WebAssembly player.
- Native, WASM and JavaScript protocol/codec tests; screen snapshots rendered from C.

## 0.3
- Page titles, full scale names, filter bars, three overlaid LFO curves and phase marker.
- ARP = SEQUENCE 1, SEQ = SEQUENCE 2; Direction / Interval / Chaos / Time.
- Root-only zero state, deterministic shuffle and timing clusters; edits preserve transport position.
- SELECT low-pass and ALGORITHM reverb with 500 ms overlay; stronger reverb send.
- Three 80 KiB samples (DMP3 headers), English uploader and browser sample storage.
- Bundled USB-MIDI firmware installer, SHA-256/size/model checks, resume and recovery link.

### 0.3 maintenance
- BPM restored to top-right header above divider.
- Appended WARM SUB: mono octave-sub bass, low-pass, mild saturation, low reverb, bass sequencer range. Existing preset IDs unchanged.

- Shifted middle-page content upward by 20 display pixels on all pages, including graphs and value overlays; header and footer retained.

- Larger lower graphs, six preset rows; SELECT tempo, ALGORITHM low-pass with response diagram; oscillator and decoded sample waveform previews on GLO.
