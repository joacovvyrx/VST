#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VoxChoirAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VoxChoirAudioProcessorEditor (VoxChoirAudioProcessor&);
    ~VoxChoirAudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void setupKnob (juce::Slider& s, juce::Label& l, const juce::String& name, const juce::String& param);
    void setupToggle (juce::ToggleButton& b, const juce::String& text, const juce::String& param);
    void setupCombo (juce::ComboBox& c, juce::Label& l, const juce::String& text, const juce::String& param);

    VoxChoirAudioProcessor& processor;
    juce::Label title, subtitle;

    juce::ComboBox choir, mic, key, scale;
    juce::Label choirL, micL, keyL, scaleL;
    juce::Slider mix, voices, spread, humanize, lowCut, brightness, reverb;
    juce::Label mixL, voicesL, spreadL, humanizeL, lowCutL, brightnessL, reverbL;
    juce::ToggleButton octUp, octDown, thirdUp, thirdDown, fifthUp, fifthDown, mono;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxChoirAudioProcessorEditor)
};
