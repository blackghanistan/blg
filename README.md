# blg — vintage degrader + reverb (VST3 / AU)

Developer: **blackghanistan** · Framework: **JUCE 8 (C++17)** · 64-bit only

## Build

```bash
git init && git submodule add https://github.com/juce-framework/JUCE.git JUCE
git -C JUCE checkout 8.0.8            # any 8.0.x tag

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

* macOS: `blg.vst3` and `blg.component` (AU, universal arm64 + x86_64) are copied to
  `~/Library/Audio/Plug-Ins/` after the build.
* Windows: the build produces the bundle folder
  `build\blg_artefacts\Release\VST3\blg.vst3` (x64). Copy the whole `blg.vst3` folder to
  `C:\Program Files\Common Files\VST3\` (or configure with `-DBLG_COPY_AFTER_BUILD=ON` in an
  administrator shell to do it automatically). Requires Visual Studio 2022 with the
  "Desktop development with C++" workload:
  `cmake -B build -G "Visual Studio 17 2022" -A x64`

### Build without installing anything (GitHub Actions)

Push this folder to a GitHub repository. `.github/workflows/build.yml` builds `blg.vst3` for
Windows x64 on every push (or run it manually from the Actions tab). Download the
`blg-vst3-windows-x64` artifact from the finished run.

### FL Studio

Copy `blg.vst3` to `C:\Program Files\Common Files\VST3\`, then in FL Studio open
Options → Manage plugins → "Find installed plugins" (with "Verify plugins" on). blg appears in
the plugin database as an effect; load it in a mixer insert slot.

## Validate

```bash
auval -v aufx Blg1 Bkgh               # macOS, AU
pluginval --strictness-level 5 build/blg_artefacts/Release/VST3/blg.vst3
```

## Signal chain

`Turbo → Sample rate + Bit depth → reconstruction low-pass → Tilt → Reverb (pre-delay + Freeverb) → Stereo (M/S)`

| Control | ID | Range | What it does |
|---|---|---|---|
| Sample rate | `sampleRate` | 1 – 48 kHz | sample & hold down-sampling + low-pass at 0.45 × rate |
| Bit depth | `bits` | 8 / 12 / 16 / 24 | quantiser (24 = off) |
| Turbo | `turbo` | 0 – 100 % | tanh drive up to +23 dB, slight bias (even harmonics), auto-level, DC blocker |
| Tilt | `tilt` | ±12 dB | spectral tilt around 650 Hz (+ = brighter) |
| Reverb Dry | `revDry` | 0 – 100 % | equal-power dry / reverb crossfade (100 % = dry only) |
| Reverb size | `revSize` | 0 – 100 % | room size |
| Width | `revWidth` | 0 – 100 % | stereo spread of the reverb tail |
| Depth | `revDepth` | 0 – 100 % | distance: pre-delay 0 – 80 ms + darker damping |
| Stereo | `stereo` | 0 – 200 % | mid/side width of the whole output (100 % = neutral) |

## Presets

Factory presets are compiled in (`PresetManager.cpp`). User presets are XML files
`<Category>/<Name>.blgpreset` in:

* macOS `~/Library/Audio/Presets/blackghanistan/blg/`
* Windows `%APPDATA%\blackghanistan\blg\`
* Linux `~/.config/blackghanistan/blg/`

## Real-time safety

* `processBlock` allocates nothing; all buffers are created in `prepareToPlay`.
* Parameters are read through atomics (`getRawParameterValue`) and smoothed per sample.
* Zero reported latency (no look-ahead, no oversampling).
* All file I/O and dialogs run on the message thread only.

## Layout

The UI is drawn on a fixed 960 × 600 canvas and scaled with a component transform, so the window
can be resized freely (640 – 1920 px wide) with the 8:5 aspect ratio locked. The window width is
stored in the plugin state.

## License note

JUCE is dual-licensed (AGPLv3 / commercial). Closed-source distribution of blg requires a JUCE
commercial licence.
