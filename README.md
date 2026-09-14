# VoxChoir VST3 (Windows)

VoxChoir is an original JUCE VST3 effect inspired by the workflow of modern voice-to-choir tools. It does **not** contain code, samples, trained models, graphics, names, or other proprietary assets from I AM CHOIR.

## What this version does

- Accepts a dry monophonic vocal as audio input.
- Builds a 4–24 voice ensemble in real time using granular pitch/delay humanisation.
- Preserves the source words and phrasing because it processes the original performance instead of using vowel samples.
- Three original profiles: Mixed Choir, Youth Choir, Gang Shout.
- Four microphone/tone profiles.
- Diatonic 3rd and 5th harmonies using a lightweight monophonic pitch tracker plus Key/Scale.
- Octave up/down layers.
- Stereo spread, humanisation, mono mode.
- Low-cut, brightness shelf and reverb.
- Delay-compensated dry/wet mix and reported plugin latency.
- VST3 + Standalone targets.

## Important limitation

This is a **DSP choir engine**, not a trained neural choir-timbre model. It is already usable as a VST, but it will sound like a sophisticated generated ensemble rather than a separately synthesized real choir. The clean next stage is to replace `GranularPitchVoice` with an authorized singing-voice/choir model (for example an ONNX-exported model trained only on recordings you have permission to use).

## Build on Windows

Requirements:

1. Windows 10/11 x64.
2. Visual Studio 2022 with **Desktop development with C++**.
3. CMake 3.22+ available in PATH.
4. Internet access during the first configure (CMake fetches JUCE 8.0.8).

Double-click:

```bat
build_windows.bat
```

The plugin should be produced at:

```text
build\VoxChoir_artefacts\Release\VST3\VoxChoir.vst3
```

Copy the `.vst3` bundle to:

```text
C:\Program Files\Common Files\VST3\
```

Then rescan plugins in Reaper, Ableton, Cubase, Studio One, etc.

## GitHub Actions

The included `.github/workflows/build-windows.yml` builds the VST3 on `windows-latest` and uploads a ZIP artifact. If you put this project in a GitHub repo, Actions can compile it without installing Visual Studio locally.

## Suggested input

Use a dry, centered, monophonic lead vocal. Heavy reverb, doubles, drums or instrumental bleed reduce pitch tracking stability. Set Choir Mix to 100% if you want the plugin to act as a choir replacement rather than a parallel layer.

## Neural engine upgrade path

For an I-AM-CHOIR-class result, the DSP stage should become:

`input vocal -> content/phoneme encoder + F0 -> authorized choir timbre model -> neural vocoder -> harmony/stem generator -> post FX`

The UI and harmony/post-FX layer in this project can remain; only the synthesis backend needs replacement.
