# TriOsc

VST3 synthesizer: 3 oscillators, ADSR, multimode filter, chorus, delay, reverb. 8 voices.

## Build

Requires CMake 3.22+ and a C++17 compiler. JUCE 8.0.8 is downloaded automatically
(or put a JUCE checkout in ./JUCE).

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

The plugin appears in build/TriOsc_artefacts/Release/VST3/TriOsc.vst3

Linux needs: libasound2-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev
libxcursor-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev

## Install

- Windows: copy TriOsc.vst3 to C:\Program Files\Common Files\VST3\
- macOS:   copy to ~/Library/Audio/Plug-Ins/VST3/
- Linux:   copy to ~/.vst3/

JUCE is licensed under AGPLv3 or a commercial license; keep that in mind if you distribute the plugin.
