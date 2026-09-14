#include "PluginEditor.h"

VoxChoirAudioProcessorEditor::VoxChoirAudioProcessorEditor (VoxChoirAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1000, 620);
    setResizable (true, true);
    setResizeLimits (850, 540, 1400, 900);

    title.setText ("VOXCHOIR", juce::dontSendNotification);
    title.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, juce::Colour (0xfff2f3f7));
    addAndMakeVisible (title);

    subtitle.setText ("real-time vocal → ensemble", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, juce::Colour (0xff8f95a8));
    addAndMakeVisible (subtitle);

    setupCombo (choir, choirL, "CHOIR", "choir");
    setupCombo (mic, micL, "MIC PROFILE", "mic");
    setupCombo (key, keyL, "KEY", "key");
    setupCombo (scale, scaleL, "SCALE", "scale");

    setupKnob (mix, mixL, "CHOIR MIX", "mix");
    setupKnob (voices, voicesL, "VOICES", "voices");
    setupKnob (spread, spreadL, "SPREAD", "spread");
    setupKnob (humanize, humanizeL, "HUMANIZE", "humanize");
    setupKnob (lowCut, lowCutL, "LOW CUT", "lowCut");
    setupKnob (brightness, brightnessL, "BRIGHTNESS", "brightness");
    setupKnob (reverb, reverbL, "REVERB", "reverb");

    setupToggle (octUp, "+ OCT", "octUp");
    setupToggle (octDown, "- OCT", "octDown");
    setupToggle (thirdUp, "+ 3RD", "thirdUp");
    setupToggle (thirdDown, "- 3RD", "thirdDown");
    setupToggle (fifthUp, "+ 5TH", "fifthUp");
    setupToggle (fifthDown, "- 5TH", "fifthDown");
    setupToggle (mono, "MONO", "mono");
}

void VoxChoirAudioProcessorEditor::setupKnob (juce::Slider& s, juce::Label& l, const juce::String& name, const juce::String& param)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffffc94a));
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff2b3040));
    s.setColour (juce::Slider::thumbColourId, juce::Colour (0xfff7f7fb));
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffd9dce6));
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, juce::Colour (0xff9ea4b8));
    l.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (l);

    sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.apvts, param, s));
}

void VoxChoirAudioProcessorEditor::setupToggle (juce::ToggleButton& b, const juce::String& text, const juce::String& param)
{
    b.setButtonText (text);
    b.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffd9dce6));
    b.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffffc94a));
    b.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff444a5e));
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.apvts, param, b));
}

void VoxChoirAudioProcessorEditor::setupCombo (juce::ComboBox& c, juce::Label& l, const juce::String& text, const juce::String& param)
{
    l.setText (text, juce::dontSendNotification);
    l.setColour (juce::Label::textColourId, juce::Colour (0xff9ea4b8));
    l.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    addAndMakeVisible (l);

    c.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff171b27));
    c.setColour (juce::ComboBox::textColourId, juce::Colour (0xffeef0f6));
    c.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff343a4e));
    addAndMakeVisible (c);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.apvts, param, c));
}

void VoxChoirAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d1018));

    auto bounds = getLocalBounds().toFloat().reduced (18.0f);
    auto header = bounds.removeFromTop (72.0f);
    juce::ignoreUnused (header);

    auto drawPanel = [&g] (juce::Rectangle<float> r, const juce::String& caption)
    {
        g.setColour (juce::Colour (0xff131722));
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (juce::Colour (0xff282e3e));
        g.drawRoundedRectangle (r, 12.0f, 1.0f);
        g.setColour (juce::Colour (0xff6f768d));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (caption, r.withHeight (26.0f).reduced (14.0f, 0.0f), juce::Justification::centredLeft);
    };

    auto content = bounds.reduced (0.0f, 4.0f);
    auto left = content.removeFromLeft (content.getWidth() * 0.34f).reduced (0.0f, 0.0f);
    content.removeFromLeft (12.0f);
    auto middle = content.removeFromLeft (content.getWidth() * 0.50f);
    content.removeFromLeft (12.0f);
    auto right = content;

    drawPanel (left, "ENSEMBLE");
    drawPanel (middle, "HARMONY GENERATOR");
    drawPanel (right, "TONE");

    g.setColour (juce::Colour (0xffffc94a));
    g.fillRoundedRectangle (20.0f, 68.0f, getWidth() - 40.0f, 2.0f, 1.0f);
}

void VoxChoirAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (24);
    auto header = r.removeFromTop (62);
    title.setBounds (header.removeFromLeft (240));
    subtitle.setBounds (header.removeFromLeft (260).reduced (4, 8));
    r.removeFromTop (16);

    const int gap = 12;
    auto left = r.removeFromLeft ((int) (r.getWidth() * 0.34f));
    r.removeFromLeft (gap);
    auto middle = r.removeFromLeft ((int) (r.getWidth() * 0.50f));
    r.removeFromLeft (gap);
    auto right = r;

    left.reduce (16, 34);
    auto row = left.removeFromTop (58);
    choirL.setBounds (row.removeFromTop (17)); choir.setBounds (row.reduced (0, 2));
    left.removeFromTop (6);
    row = left.removeFromTop (58);
    micL.setBounds (row.removeFromTop (17)); mic.setBounds (row.reduced (0, 2));
    left.removeFromTop (12);

    auto knobs = left.removeFromTop (150);
    const int kw = knobs.getWidth() / 2;
    auto k1 = knobs.removeFromLeft (kw);
    voicesL.setBounds (k1.removeFromTop (20)); voices.setBounds (k1.reduced (4));
    spreadL.setBounds (knobs.removeFromTop (20)); spread.setBounds (knobs.reduced (4));
    left.removeFromTop (6);
    auto bottomKnobs = left.removeFromTop (150);
    auto b1 = bottomKnobs.removeFromLeft (kw);
    humanizeL.setBounds (b1.removeFromTop (20)); humanize.setBounds (b1.reduced (4));
    mixL.setBounds (bottomKnobs.removeFromTop (20)); mix.setBounds (bottomKnobs.reduced (4));

    middle.reduce (16, 34);
    auto top = middle.removeFromTop (58);
    auto keyArea = top.removeFromLeft (top.getWidth() / 2 - 5);
    top.removeFromLeft (10);
    keyL.setBounds (keyArea.removeFromTop (17)); key.setBounds (keyArea.reduced (0, 2));
    scaleL.setBounds (top.removeFromTop (17)); scale.setBounds (top.reduced (0, 2));
    middle.removeFromTop (18);

    auto toggles = middle.removeFromTop (225);
    const int tw = toggles.getWidth() / 2;
    const int th = 52;
    auto putPair = [&] (juce::ToggleButton& a, juce::ToggleButton& b)
    {
        auto rr = toggles.removeFromTop (th);
        auto aa = rr.removeFromLeft (tw);
        a.setBounds (aa.reduced (5));
        b.setBounds (rr.reduced (5));
    };
    putPair (octUp, octDown);
    putPair (thirdUp, thirdDown);
    putPair (fifthUp, fifthDown);
    mono.setBounds (toggles.removeFromTop (th).withSizeKeepingCentre (140, 42));

    right.reduce (14, 34);
    auto placeTone = [&] (juce::Label& l, juce::Slider& s)
    {
        auto rr = right.removeFromTop (145);
        l.setBounds (rr.removeFromTop (20));
        s.setBounds (rr.reduced (0, 2));
        right.removeFromTop (3);
    };
    placeTone (lowCutL, lowCut);
    placeTone (brightnessL, brightness);
    placeTone (reverbL, reverb);
}
