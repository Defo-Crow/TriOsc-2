#pragma once
#include "Common.h"
#include "Parameters.h"
#include "SynthEngine.h"
#include "Effects.h"

class TriOscProcessor : public juce::AudioProcessor
{
public:
    TriOscProcessor();
    ~TriOscProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TriOsc"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int size) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    float get (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    tri::SynthParams readSynthParams() const;

    tri::SynthEngine engine;
    juce::dsp::Chorus<float> chorus;
    tri::StereoDelay delay;
    juce::Reverb reverb;
    juce::AudioBuffer<float> wetBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterSm;
    int maxBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriOscProcessor)
};
