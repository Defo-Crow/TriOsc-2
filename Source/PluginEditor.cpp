#include "PluginEditor.h"

namespace
{
const juce::Colour kBg      { 0xff14161c };
const juce::Colour kPanel   { 0xff1e222b };
const juce::Colour kAccent  { 0xff4fd1c5 };
const juce::Colour kText    { 0xffd8dee9 };
const juce::Colour kMuted   { 0xff7d879a };
const juce::Colour kOutline { 0xff3a4150 };
}

TriLookAndFeel::TriLookAndFeel()
{
    setColourScheme (getDarkColourScheme());
    setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    setColour (juce::Slider::rotarySliderOutlineColourId, kOutline);
    setColour (juce::Slider::thumbColourId, kText);
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, kText);
    setColour (juce::ComboBox::backgroundColourId, kBg);
    setColour (juce::ComboBox::outlineColourId, kOutline);
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::arrowColourId, kAccent);
    setColour (juce::ToggleButton::tickColourId, kAccent);
    setColour (juce::ToggleButton::textColourId, kText);
    setColour (juce::ToggleButton::tickDisabledColourId, kMuted);
    setColour (juce::PopupMenu::backgroundColourId, kPanel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kAccent.withAlpha (0.3f));
}

// ---------------------------------------------------------------- Knob

Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kMuted);
    addAndMakeVisible (label);

    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 16);
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    const int w = juce::jmin (r.getWidth(), 84);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, juce::jmin (w, 72), 16);
    slider.setBounds (r.withSizeKeepingCentre (w, r.getHeight()));
}

// --------------------------------------------------------------- Panel

void Panel::addKnob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text)
{
    auto k = std::make_unique<Knob> (s, id, text);
    addAndMakeVisible (*k);
    items.push_back ({ k.get(), 1.0f, true });
    owned.push_back (std::move (k));
}

void Panel::addCombo (juce::AudioProcessorValueTreeState& s, const juce::String& id, float weight)
{
    auto c = std::make_unique<juce::ComboBox>();
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        c->addItemList (p->choices, 1);
    addAndMakeVisible (*c);
    comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, *c));
    items.push_back ({ c.get(), weight, false });
    owned.push_back (std::move (c));
}

void Panel::addToggle (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, float weight)
{
    auto b = std::make_unique<juce::ToggleButton> (text);
    addAndMakeVisible (*b);
    buttonAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *b));
    items.push_back ({ b.get(), weight, false });
    owned.push_back (std::move (b));
}

float Panel::totalWeight() const
{
    float t = 0.0f;
    for (auto& i : items) t += i.weight;
    return t;
}

void Panel::paint (juce::Graphics& g)
{
    g.setColour (kPanel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    g.setColour (kOutline);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
    g.setColour (kAccent);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText (title, 12, 6, getWidth() - 24, 16, juce::Justification::left);
}

void Panel::resized()
{
    auto area = getLocalBounds().reduced (8, 6);
    area.removeFromTop (22);
    const float total = totalWeight();
    if (total <= 0.0f) return;

    float x = (float) area.getX();
    for (auto& it : items)
    {
        const float w = (float) area.getWidth() * it.weight / total;
        auto cell = juce::Rectangle<float> (x, (float) area.getY(), w, (float) area.getHeight()).toNearestInt();
        if (it.isKnob)
            it.comp->setBounds (cell);
        else
            it.comp->setBounds (cell.reduced (3, 0).withSizeKeepingCentre (cell.getWidth() - 6, 26)
                                    .withY (area.getY() + (int) (area.getHeight() * 0.38f)));
        x += w;
    }
}

// -------------------------------------------------------------- Editor

TriOscEditor::TriOscEditor (TriOscProcessor& p)
    : AudioProcessorEditor (&p)
{
    setLookAndFeel (&laf);
    auto& s = p.apvts;

    Panel* oscs[3] = { &osc1, &osc2, &osc3 };
    for (int i = 0; i < 3; ++i)
    {
        oscs[i]->addCombo (s, tri::oscId (i, "wave"), 1.5f);
        oscs[i]->addKnob (s, tri::oscId (i, "oct"), "Octave");
        oscs[i]->addKnob (s, tri::oscId (i, "semi"), "Semi");
        oscs[i]->addKnob (s, tri::oscId (i, "fine"), "Fine");
        oscs[i]->addKnob (s, tri::oscId (i, "level"), "Level");
    }

    amp.addKnob (s, "amp_a", "Attack");
    amp.addKnob (s, "amp_d", "Decay");
    amp.addKnob (s, "amp_s", "Sustain");
    amp.addKnob (s, "amp_r", "Release");

    filter.addCombo (s, "f_type", 1.5f);
    filter.addKnob (s, "f_cut", "Cutoff");
    filter.addKnob (s, "f_res", "Reso");
    filter.addKnob (s, "f_env", "Env Amt");
    filter.addKnob (s, "f_dec", "Env Decay");

    master.addKnob (s, "master", "Volume");

    chorus.addKnob (s, "ch_rate", "Rate");
    chorus.addKnob (s, "ch_depth", "Depth");
    chorus.addKnob (s, "ch_mix", "Mix");

    delay.addKnob (s, "dl_time", "Time");
    delay.addKnob (s, "dl_fb", "Feedback");
    delay.addKnob (s, "dl_mix", "Mix");
    delay.addToggle (s, "dl_pp", "Ping-pong", 1.3f);

    reverb.addKnob (s, "rv_size", "Size");
    reverb.addKnob (s, "rv_damp", "Damping");
    reverb.addKnob (s, "rv_mix", "Mix");

    for (auto* c : { &osc1, &osc2, &osc3, &amp, &filter, &master, &chorus, &delay, &reverb })
        addAndMakeVisible (*c);

    setSize (960, 600);
}

TriOscEditor::~TriOscEditor()
{
    setLookAndFeel (nullptr);
}

void TriOscEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kText);
    g.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    g.drawText ("TRI-OSC", 16, 10, 200, 34, juce::Justification::left);
    g.setColour (kMuted);
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText ("3 oscillators  /  chorus  /  delay  /  reverb", getWidth() - 416, 18, 400, 20, juce::Justification::right);
}

void TriOscEditor::layoutRow (juce::Rectangle<int> row, std::initializer_list<Panel*> panels)
{
    const int gap = 10;
    float sum = 0.0f;
    for (auto* p : panels) sum += p->totalWeight();
    const float avail = (float) (row.getWidth() - gap * ((int) panels.size() - 1));

    float x = (float) row.getX();
    for (auto* p : panels)
    {
        const float w = avail * p->totalWeight() / sum;
        p->setBounds (juce::Rectangle<float> (x, (float) row.getY(), w, (float) row.getHeight()).toNearestInt());
        x += w + (float) gap;
    }
}

void TriOscEditor::resized()
{
    auto r = getLocalBounds().reduced (16, 0);
    r.removeFromTop (56);
    r.removeFromBottom (16);
    const int gap = 10;
    const int rowH = (r.getHeight() - 2 * gap) / 3;

    layoutRow (r.removeFromTop (rowH), { &osc1, &osc2, &osc3 });
    r.removeFromTop (gap);
    layoutRow (r.removeFromTop (rowH), { &amp, &filter, &master });
    r.removeFromTop (gap);
    layoutRow (r.removeFromTop (rowH), { &chorus, &delay, &reverb });
}
