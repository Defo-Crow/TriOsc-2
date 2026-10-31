#pragma once
#include "PluginProcessor.h"

class TriLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TriLookAndFeel();
    juce::Font getLabelFont (juce::Label&) override { return juce::Font (juce::FontOptions (12.5f)); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.5f)); }
    juce::Font getPopupMenuFont() override { return juce::Font (juce::FontOptions (13.0f)); }
};

class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& text);
    void resized() override;

private:
    juce::Label label;
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class Panel : public juce::Component
{
public:
    explicit Panel (juce::String t) : title (std::move (t)) {}

    void addKnob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text);
    void addCombo (juce::AudioProcessorValueTreeState& s, const juce::String& id, float weight);
    void addToggle (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, float weight);

    float totalWeight() const;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Item { juce::Component* comp; float weight; bool isKnob; };

    juce::String title;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<Item> items;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAtt;
};

class TriOscEditor : public juce::AudioProcessorEditor
{
public:
    explicit TriOscEditor (TriOscProcessor&);
    ~TriOscEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void layoutRow (juce::Rectangle<int> row, std::initializer_list<Panel*> panels);

    TriLookAndFeel laf;
    Panel osc1 { "OSC 1" }, osc2 { "OSC 2" }, osc3 { "OSC 3" };
    Panel amp { "AMP ENVELOPE" }, filter { "FILTER" }, master { "OUT" };
    Panel chorus { "CHORUS" }, delay { "DELAY" }, reverb { "REVERB" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriOscEditor)
};
