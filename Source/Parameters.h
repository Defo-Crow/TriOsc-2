#pragma once
#include "Common.h"

namespace tri
{
inline juce::String oscId (int i, const juce::String& name)
{
    return "osc" + juce::String (i + 1) + "_" + name;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pid = [] (const String& id) { return ParameterID { id, 1 }; };

    auto txt = [] (int decimals, const String& unit, bool sign = false)
    {
        return [=] (float v, int) {
            return String ((sign && v > 0.0f) ? "+" : "") + String (v, decimals) + unit;
        };
    };

    auto addFloat = [&] (const String& id, const String& name, float lo, float hi, float def,
                         float skew, std::function<String (float, int)> text)
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            pid (id), name, NormalisableRange<float> (lo, hi, 0.0f, skew), def,
            AudioParameterFloatAttributes().withStringFromValueFunction (std::move (text))));
    };

    const StringArray waves { "Sine", "Saw", "Square", "Triangle" };
    const int defWave[3]  = { 1, 1, 2 };
    const int defOct[3]   = { 0, 0, -1 };
    const float defFine[3] = { 0.0f, 9.0f, 0.0f };
    const float defLvl[3] = { 0.8f, 0.6f, 0.5f };

    for (int i = 0; i < 3; ++i)
    {
        const String n = "Osc " + String (i + 1) + " ";
        layout.add (std::make_unique<AudioParameterChoice> (pid (oscId (i, "wave")), n + "Wave", waves, defWave[i]));
        layout.add (std::make_unique<AudioParameterInt> (pid (oscId (i, "oct")), n + "Octave", -2, 2, defOct[i],
            AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return String (v > 0 ? "+" : "") + String (v); })));
        layout.add (std::make_unique<AudioParameterInt> (pid (oscId (i, "semi")), n + "Semitone", -12, 12, 0,
            AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return String (v > 0 ? "+" : "") + String (v); })));
        addFloat (oscId (i, "fine"), n + "Fine", -50.0f, 50.0f, defFine[i], 1.0f, txt (0, " ct", true));
        addFloat (oscId (i, "level"), n + "Level", 0.0f, 1.0f, defLvl[i], 1.0f, txt (2, ""));
    }

    addFloat ("amp_a", "Attack",  0.001f, 5.0f, 0.01f, 0.35f, txt (3, " s"));
    addFloat ("amp_d", "Decay",   0.001f, 5.0f, 0.30f, 0.35f, txt (2, " s"));
    addFloat ("amp_s", "Sustain", 0.0f,   1.0f, 0.70f, 1.0f,  txt (2, ""));
    addFloat ("amp_r", "Release", 0.001f, 8.0f, 0.40f, 0.35f, txt (2, " s"));

    layout.add (std::make_unique<AudioParameterChoice> (pid ("f_type"), "Filter Type",
                                                        StringArray { "Low-pass", "High-pass", "Band-pass" }, 0));
    addFloat ("f_cut", "Cutoff",    20.0f, 20000.0f, 4000.0f, 0.25f, [] (float v, int) {
        return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String ((int) v) + " Hz"; });
    addFloat ("f_res", "Resonance", 0.0f, 1.0f, 0.2f, 1.0f, txt (2, ""));
    addFloat ("f_env", "Env Amount", -1.0f, 1.0f, 0.3f, 1.0f, txt (2, "", true));
    addFloat ("f_dec", "Env Decay", 0.01f, 4.0f, 0.5f, 0.4f, txt (2, " s"));

    addFloat ("master", "Master", 0.0f, 1.0f, 0.7f, 1.0f, txt (2, ""));

    addFloat ("ch_rate",  "Chorus Rate",  0.05f, 8.0f, 0.8f, 0.4f, txt (2, " Hz"));
    addFloat ("ch_depth", "Chorus Depth", 0.0f, 1.0f, 0.4f, 1.0f, txt (2, ""));
    addFloat ("ch_mix",   "Chorus Mix",   0.0f, 1.0f, 0.3f, 1.0f, txt (2, ""));

    addFloat ("dl_time", "Delay Time",     10.0f, 1000.0f, 350.0f, 0.5f, txt (0, " ms"));
    addFloat ("dl_fb",   "Delay Feedback", 0.0f, 0.95f, 0.4f, 1.0f, txt (2, ""));
    addFloat ("dl_mix",  "Delay Mix",      0.0f, 1.0f, 0.2f, 1.0f, txt (2, ""));
    layout.add (std::make_unique<AudioParameterBool> (pid ("dl_pp"), "Delay Ping-Pong", true));

    addFloat ("rv_size", "Reverb Size",    0.0f, 1.0f, 0.6f, 1.0f, txt (2, ""));
    addFloat ("rv_damp", "Reverb Damping", 0.0f, 1.0f, 0.5f, 1.0f, txt (2, ""));
    addFloat ("rv_mix",  "Reverb Mix",     0.0f, 1.0f, 0.25f, 1.0f, txt (2, ""));

    return layout;
}
} // namespace tri
