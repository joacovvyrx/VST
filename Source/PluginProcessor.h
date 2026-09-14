#pragma once
#include <JuceHeader.h>
#include "ChoirDSP.h"

class VoxChoirAudioProcessor : public juce::AudioProcessor
{
public:
    VoxChoirAudioProcessor();
    ~VoxChoirAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    enum class Part { unison, octaveUp, octaveDown, thirdUp, thirdDown, fifthUp, fifthDown };

    float getPartSemitones (Part p, int midiNote, int key, bool minor) const;
    void updatePostFilters (float lowCutHz, float brightnessDb, int choirType, int micType);

    static constexpr int maxVoices = 24;
    std::array<GranularPitchVoice, maxVoices> pitchVoices;
    PitchTracker pitchTracker;
    SimpleDelay dryDelayL, dryDelayR;

    juce::dsp::IIR::Filter<float> highPassL, highPassR;
    juce::dsp::IIR::Filter<float> shelfL, shelfR;
    juce::Reverb reverb;

    juce::AudioBuffer<float> wetBuffer;
    double currentSampleRate = 48000.0;
    int latencySamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxChoirAudioProcessor)
};
