#pragma once
#include "Common.h"

namespace tri
{
struct OscParams
{
    int wave = 1;        // 0 sine, 1 saw, 2 square, 3 triangle
    float semis = 0.0f;  // octave*12 + semitone + fine/100
    float level = 0.0f;
};

struct SynthParams
{
    OscParams osc[3];
    float ampA = 0.01f, ampD = 0.3f, ampS = 0.7f, ampR = 0.4f;
    int filterType = 0;  // 0 low-pass, 1 high-pass, 2 band-pass
    float cutoff = 4000.0f, q = 1.0f, envAmount = 0.0f, envDecay = 0.5f;
};

inline float polyBlep (float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

inline float oscSample (int wave, float p, float dt)
{
    switch (wave)
    {
        case 0:  return std::sin (juce::MathConstants<float>::twoPi * p);
        case 1:  return 2.0f * p - 1.0f - polyBlep (p, dt);
        case 2:
        {
            float p2 = p + 0.5f;
            if (p2 >= 1.0f) p2 -= 1.0f;
            return (p < 0.5f ? 1.0f : -1.0f) + polyBlep (p, dt) - polyBlep (p2, dt);
        }
        default: return 4.0f * std::abs (p - 0.5f) - 1.0f;
    }
}

class Voice
{
public:
    void prepare (double sr, int maxBlock)
    {
        sampleRate = sr;
        filter.prepare ({ sr, (juce::uint32) juce::jmax (1, maxBlock), 1 });
        amp.setSampleRate (sr);
        reset();
    }

    void reset()
    {
        active = false;
        released = true;
        keyDown = false;
        amp.reset();
        filter.reset();
    }

    bool isActive() const noexcept { return active; }
    bool isReleased() const noexcept { return released; }
    bool isKeyDown() const noexcept { return keyDown; }
    int getNote() const noexcept { return midiNote; }
    juce::uint64 getOrder() const noexcept { return order; }
    void setKeyDown (bool d) noexcept { keyDown = d; }

    void start (int note, float vel, const SynthParams& p, juce::uint64 startOrder)
    {
        midiNote = note;
        velocity = vel;
        order = startOrder;
        keyDown = true;
        released = false;
        active = true;
        phase[0] = phase[1] = phase[2] = 0.0f;
        fEnv = 1.0f;
        filter.reset();
        amp.setParameters ({ p.ampA, p.ampD, p.ampS, p.ampR });
        amp.noteOn();
    }

    void release()
    {
        if (! released)
        {
            released = true;
            amp.noteOff();
        }
    }

    void render (float* l, float* r, int n, const SynthParams& p, float bendSemis)
    {
        if (! active) return;

        amp.setParameters ({ p.ampA, p.ampD, p.ampS, p.ampR });
        const float envCoef = std::exp (-6.9f / (std::max (0.005f, p.envDecay) * (float) sampleRate));
        const float nyquistLimit = 0.45f * (float) sampleRate;

        int i = 0;
        while (i < n)
        {
            const int chunk = std::min (kControl, n - i);

            float inc[3];
            for (int o = 0; o < 3; ++o)
            {
                const float f = 440.0f * std::pow (2.0f, ((float) (midiNote - 69) + p.osc[o].semis + bendSemis) / 12.0f);
                inc[o] = juce::jlimit (0.0f, 0.45f, f / (float) sampleRate);
            }

            const float cut = juce::jlimit (20.0f, nyquistLimit, p.cutoff * std::pow (2.0f, p.envAmount * 5.0f * fEnv));
            filter.setType (p.filterType == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                          : p.filterType == 1 ? juce::dsp::StateVariableTPTFilterType::highpass
                                              : juce::dsp::StateVariableTPTFilterType::bandpass);
            filter.setCutoffFrequency (cut);
            filter.setResonance (p.q);

            for (int k = 0; k < chunk; ++k)
            {
                float s = 0.0f;
                for (int o = 0; o < 3; ++o)
                {
                    const float lvl = p.osc[o].level;
                    if (lvl > 0.0f)
                        s += oscSample (p.osc[o].wave, phase[o], inc[o]) * lvl;
                    phase[o] += inc[o];
                    if (phase[o] >= 1.0f) phase[o] -= 1.0f;
                }

                s = filter.processSample (0, s * 0.45f);
                const float out = s * amp.getNextSample() * velocity;
                l[i + k] += out;
                r[i + k] += out;

                if (! amp.isActive())
                {
                    active = false;
                    return;
                }
            }

            fEnv *= std::pow (envCoef, (float) chunk);
            i += chunk;
        }
    }

private:
    static constexpr int kControl = 32;
    double sampleRate = 44100.0;
    bool active = false, released = true, keyDown = false;
    int midiNote = 60;
    float velocity = 1.0f, fEnv = 0.0f;
    float phase[3] { 0, 0, 0 };
    juce::uint64 order = 0;
    juce::ADSR amp;
    juce::dsp::StateVariableTPTFilter<float> filter;
};

class SynthEngine
{
public:
    static constexpr int kVoices = 8;

    void prepare (double sr, int maxBlock)
    {
        for (auto& v : voices) v.prepare (sr, maxBlock);
        bend = 0.0f;
        sustain = false;
    }

    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const SynthParams& p)
    {
        int pos = 0;
        const int total = buffer.getNumSamples();

        for (const auto meta : midi)
        {
            const int evPos = juce::jlimit (0, total, meta.samplePosition);
            if (evPos > pos)
            {
                renderVoices (buffer, pos, evPos - pos, p);
                pos = evPos;
            }
            handle (meta.getMessage(), p);
        }
        if (pos < total)
            renderVoices (buffer, pos, total - pos, p);
    }

private:
    void renderVoices (juce::AudioBuffer<float>& buffer, int start, int count, const SynthParams& p)
    {
        float* l = buffer.getWritePointer (0) + start;
        float* r = buffer.getWritePointer (1) + start;
        for (auto& v : voices)
            v.render (l, r, count, p, bend);
    }

    void handle (const juce::MidiMessage& m, const SynthParams& p)
    {
        if (m.isNoteOn())
        {
            Voice* target = nullptr;
            for (auto& v : voices)
                if (! v.isActive()) { target = &v; break; }
            if (target == nullptr)
            {
                target = &voices[0];
                for (auto& v : voices)
                    if (v.getOrder() < target->getOrder()) target = &v;
            }
            target->start (m.getNoteNumber(), m.getFloatVelocity(), p, ++counter);
        }
        else if (m.isNoteOff())
        {
            for (auto& v : voices)
                if (v.isActive() && v.isKeyDown() && v.getNote() == m.getNoteNumber())
                {
                    v.setKeyDown (false);
                    if (! sustain) v.release();
                }
        }
        else if (m.isPitchWheel())
        {
            bend = ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * 2.0f;
        }
        else if (m.isSustainPedalOn())
        {
            sustain = true;
        }
        else if (m.isSustainPedalOff())
        {
            sustain = false;
            for (auto& v : voices)
                if (v.isActive() && ! v.isKeyDown()) v.release();
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            for (auto& v : voices) v.reset();
            sustain = false;
        }
    }

    Voice voices[kVoices];
    juce::uint64 counter = 0;
    float bend = 0.0f;
    bool sustain = false;
};
} // namespace tri
