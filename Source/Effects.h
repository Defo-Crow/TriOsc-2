#pragma once
#include "Common.h"

namespace tri
{
// Stereo delay with a darkened feedback path and optional ping-pong routing.
class StereoDelay
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        size = (int) (sr * 1.2) + 4;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        writeIdx = 0;
        fbL = fbR = 0.0f;
        lpCoef = std::exp (-juce::MathConstants<float>::twoPi * 5000.0f / (float) sr);
        timeSm.reset (sr, 0.08);
        timeSm.setCurrentAndTargetValue (timeMs);
    }

    void setParams (float time, float feedbackAmt, float mixAmt, bool pp)
    {
        timeMs = time;
        timeSm.setTargetValue (time);
        feedback = feedbackAmt;
        mix = mixAmt;
        pingPong = pp;
    }

    void process (float* l, float* r, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            float t = timeSm.getNextValue() * 0.001f * (float) sampleRate;
            t = juce::jlimit (1.0f, (float) (size - 2), t);
            float rp = (float) writeIdx - t;
            if (rp < 0.0f) rp += (float) size;
            const int i0 = (int) rp;
            const float fr = rp - (float) i0;
            int i1 = i0 + 1;
            if (i1 >= size) i1 -= size;

            const float dl = bufL[(size_t) i0] * (1.0f - fr) + bufL[(size_t) i1] * fr;
            const float dr = bufR[(size_t) i0] * (1.0f - fr) + bufR[(size_t) i1] * fr;

            fbL = (1.0f - lpCoef) * dl + lpCoef * fbL;
            fbR = (1.0f - lpCoef) * dr + lpCoef * fbR;

            const float inL = l[i], inR = r[i];
            if (pingPong)
            {
                const float m = 0.5f * (inL + inR);
                bufL[(size_t) writeIdx] = m + feedback * fbR;
                bufR[(size_t) writeIdx] = feedback * fbL;
            }
            else
            {
                bufL[(size_t) writeIdx] = inL + feedback * fbL;
                bufR[(size_t) writeIdx] = inR + feedback * fbR;
            }

            l[i] = inL + mix * dl;
            r[i] = inR + mix * dr;

            if (++writeIdx >= size) writeIdx = 0;
        }
    }

private:
    double sampleRate = 44100.0;
    int size = 1, writeIdx = 0;
    std::vector<float> bufL, bufR;
    float fbL = 0, fbR = 0, lpCoef = 0.0f;
    float timeMs = 350.0f, feedback = 0.4f, mix = 0.2f;
    bool pingPong = true;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> timeSm;
};
} // namespace tri
