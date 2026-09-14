#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxChoirAudioProcessor::VoxChoirAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxChoirAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<APF> (juce::ParameterID { "mix", 1 }, "Choir Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    p.push_back (std::make_unique<APC> (juce::ParameterID { "choir", 1 }, "Choir",
        juce::StringArray { "Mixed Choir", "Youth Choir", "Gang Shout" }, 0));
    p.push_back (std::make_unique<APC> (juce::ParameterID { "mic", 1 }, "Mic",
        juce::StringArray { "Neutral", "Vintage 67", "Tube 12", "Tight SDC" }, 0));
    p.push_back (std::make_unique<API> (juce::ParameterID { "voices", 1 }, "Voices", 4, maxVoices, 12));
    p.push_back (std::make_unique<APF> (juce::ParameterID { "spread", 1 }, "Stereo Spread",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 78.0f));
    p.push_back (std::make_unique<APF> (juce::ParameterID { "humanize", 1 }, "Humanize",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f));

    p.push_back (std::make_unique<APB> (juce::ParameterID { "octUp", 1 }, "Octave Up", false));
    p.push_back (std::make_unique<APB> (juce::ParameterID { "octDown", 1 }, "Octave Down", false));
    p.push_back (std::make_unique<APB> (juce::ParameterID { "thirdUp", 1 }, "Third Above", false));
    p.push_back (std::make_unique<APB> (juce::ParameterID { "thirdDown", 1 }, "Third Below", false));
    p.push_back (std::make_unique<APB> (juce::ParameterID { "fifthUp", 1 }, "Fifth Above", false));
    p.push_back (std::make_unique<APB> (juce::ParameterID { "fifthDown", 1 }, "Fifth Below", false));

    p.push_back (std::make_unique<APC> (juce::ParameterID { "key", 1 }, "Key",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    p.push_back (std::make_unique<APC> (juce::ParameterID { "scale", 1 }, "Scale",
        juce::StringArray { "Major", "Minor" }, 0));
    p.push_back (std::make_unique<APB> (juce::ParameterID { "mono", 1 }, "Mono", false));

    p.push_back (std::make_unique<APF> (juce::ParameterID { "lowCut", 1 }, "Low Cut",
        juce::NormalisableRange<float> (20.0f, 300.0f, 0.1f, 0.45f), 65.0f));
    p.push_back (std::make_unique<APF> (juce::ParameterID { "brightness", 1 }, "Brightness",
        juce::NormalisableRange<float> (-6.0f, 10.0f, 0.1f), 1.5f));
    p.push_back (std::make_unique<APF> (juce::ParameterID { "reverb", 1 }, "Reverb",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 22.0f));

    return { p.begin(), p.end() };
}

void VoxChoirAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    for (auto& v : pitchVoices) v.prepare (sampleRate);
    pitchTracker.prepare (sampleRate);

    latencySamples = pitchVoices.front().getLatencySamples();
    setLatencySamples (latencySamples);
    dryDelayL.prepare (latencySamples + (int) (sampleRate * 0.1));
    dryDelayR.prepare (latencySamples + (int) (sampleRate * 0.1));

    wetBuffer.setSize (2, samplesPerBlock, false, false, true);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
}

void VoxChoirAudioProcessor::releaseResources()
{
    reverb.reset();
}

bool VoxChoirAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == out;
}

float VoxChoirAudioProcessor::getPartSemitones (Part p, int midiNote, int key, bool minor) const
{
    switch (p)
    {
        case Part::unison:     return 0.0f;
        case Part::octaveUp:   return 12.0f;
        case Part::octaveDown: return -12.0f;
        case Part::thirdUp:    return (float) diatonicSemitoneOffset (midiNote, key, minor, 2);
        case Part::thirdDown:  return (float) diatonicSemitoneOffset (midiNote, key, minor, -2);
        case Part::fifthUp:    return (float) diatonicSemitoneOffset (midiNote, key, minor, 4);
        case Part::fifthDown:  return (float) diatonicSemitoneOffset (midiNote, key, minor, -4);
    }
    return 0.0f;
}

void VoxChoirAudioProcessor::updatePostFilters (float lowCutHz, float brightnessDb, int choirType, int micType)
{
    float profileBrightness = 0.0f;
    float hpBoost = 0.0f;

    if (choirType == 1) { profileBrightness += 2.7f; hpBoost += 35.0f; }
    if (choirType == 2) { profileBrightness -= 1.8f; hpBoost += 10.0f; }

    switch (micType)
    {
        case 1: profileBrightness -= 1.2f; break; // warm
        case 2: profileBrightness += 1.4f; break; // open tube
        case 3: profileBrightness += 2.2f; hpBoost += 25.0f; break;
        default: break;
    }

    const auto hp = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate,
        juce::jlimit (20.0f, 450.0f, lowCutHz + hpBoost));
    highPassL.coefficients = hp;
    highPassR.coefficients = hp;

    const float shelfGain = juce::Decibels::decibelsToGain (juce::jlimit (-12.0f, 14.0f, brightnessDb + profileBrightness));
    const auto sh = juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 4200.0, 0.707, shelfGain);
    shelfL.coefficients = sh;
    shelfR.coefficients = sh;
}

void VoxChoirAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int inChannels = getTotalNumInputChannels();
    const int outChannels = getTotalNumOutputChannels();
    for (int ch = inChannels; ch < outChannels; ++ch) buffer.clear (ch, 0, numSamples);

    wetBuffer.setSize (2, numSamples, false, false, true);
    wetBuffer.clear();

    const float mix = apvts.getRawParameterValue ("mix")->load() * 0.01f;
    const int choirType = (int) apvts.getRawParameterValue ("choir")->load();
    const int micType = (int) apvts.getRawParameterValue ("mic")->load();
    const int requestedVoices = (int) apvts.getRawParameterValue ("voices")->load();
    const float spread = apvts.getRawParameterValue ("spread")->load() * 0.01f;
    const float humanize = apvts.getRawParameterValue ("humanize")->load() * 0.01f;
    const int key = (int) apvts.getRawParameterValue ("key")->load();
    const bool minor = apvts.getRawParameterValue ("scale")->load() > 0.5f;
    const bool forceMono = apvts.getRawParameterValue ("mono")->load() > 0.5f;

    std::array<Part, 7> parts {};
    int partCount = 0;
    parts[(size_t) partCount++] = Part::unison;
    if (apvts.getRawParameterValue ("octUp")->load() > 0.5f) parts[(size_t) partCount++] = Part::octaveUp;
    if (apvts.getRawParameterValue ("octDown")->load() > 0.5f) parts[(size_t) partCount++] = Part::octaveDown;
    if (apvts.getRawParameterValue ("thirdUp")->load() > 0.5f) parts[(size_t) partCount++] = Part::thirdUp;
    if (apvts.getRawParameterValue ("thirdDown")->load() > 0.5f) parts[(size_t) partCount++] = Part::thirdDown;
    if (apvts.getRawParameterValue ("fifthUp")->load() > 0.5f) parts[(size_t) partCount++] = Part::fifthUp;
    if (apvts.getRawParameterValue ("fifthDown")->load() > 0.5f) parts[(size_t) partCount++] = Part::fifthDown;

    const int activeVoices = juce::jlimit (juce::jmax (partCount, 4), maxVoices, requestedVoices);
    static constexpr float detunePattern[24] = {
        -15, 11, -7, 5, -3, 2, -11, 14, -5, 8, -1, 4,
        -13, 9, -4, 6, -8, 12, -2, 3, -10, 7, -6, 10
    };
    static constexpr float panPattern[24] = {
        -0.94f, 0.86f, -0.55f, 0.62f, -0.24f, 0.31f, -0.73f, 0.76f,
        -0.42f, 0.48f, -0.12f, 0.16f, -0.82f, 0.92f, -0.34f, 0.38f,
        -0.66f, 0.69f, -0.07f, 0.09f, -0.49f, 0.53f, -0.20f, 0.25f
    };

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = buffer.getSample (0, i);
        const float r = inChannels > 1 ? buffer.getSample (1, i) : l;
        const float monoIn = 0.5f * (l + r);
        const int midi = pitchTracker.processSample (monoIn);

        float wetL = 0.0f, wetR = 0.0f;
        for (int v = 0; v < activeVoices; ++v)
        {
            const Part part = parts[(size_t) (v % partCount)];
            float semi = getPartSemitones (part, midi, key, minor);
            float detuneCents = detunePattern[v] * humanize;
            float delayMs = (2.0f + (float) ((v * 17) % 31)) * humanize;

            // Profiles alter ensemble behaviour without changing the sung melody.
            if (choirType == 1) { detuneCents *= 0.72f; delayMs *= 0.72f; }
            if (choirType == 2) { detuneCents *= 1.22f; delayMs *= 1.25f; }

            semi += detuneCents / 100.0f;
            const float y = pitchVoices[(size_t) v].process (monoIn, semi, delayMs);
            float pan = forceMono ? 0.0f : panPattern[v] * spread;
            if (choirType == 2) pan *= 0.78f;
            const float angle = (pan + 1.0f) * (juce::MathConstants<float>::pi * 0.25f);
            wetL += y * std::cos (angle);
            wetR += y * std::sin (angle);
        }

        const float norm = 1.0f / std::sqrt ((float) activeVoices);
        wetL *= norm;
        wetR *= norm;

        if (choirType == 2)
        {
            wetL = std::tanh (wetL * 1.45f) * 0.86f;
            wetR = std::tanh (wetR * 1.45f) * 0.86f;
        }

        wetBuffer.setSample (0, i, wetL);
        wetBuffer.setSample (1, i, wetR);

        const float dryL = dryDelayL.process (l, latencySamples);
        const float dryR = dryDelayR.process (r, latencySamples);
        buffer.setSample (0, i, dryL);
        if (outChannels > 1) buffer.setSample (1, i, dryR);
    }

    updatePostFilters (apvts.getRawParameterValue ("lowCut")->load(),
                       apvts.getRawParameterValue ("brightness")->load(), choirType, micType);

    auto* wl = wetBuffer.getWritePointer (0);
    auto* wr = wetBuffer.getWritePointer (1);
    for (int i = 0; i < numSamples; ++i)
    {
        wl[i] = shelfL.processSample (highPassL.processSample (wl[i]));
        wr[i] = shelfR.processSample (highPassR.processSample (wr[i]));
    }

    juce::Reverb::Parameters rp;
    const float reverbAmt = apvts.getRawParameterValue ("reverb")->load() * 0.01f;
    rp.roomSize = 0.62f + 0.25f * reverbAmt;
    rp.damping = 0.42f;
    rp.wetLevel = 0.10f + 0.42f * reverbAmt;
    rp.dryLevel = 1.0f - 0.16f * reverbAmt;
    rp.width = forceMono ? 0.0f : 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.processStereo (wl, wr, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const float outL = buffer.getSample (0, i) * (1.0f - mix) + wl[i] * mix;
        const float outR = (outChannels > 1 ? buffer.getSample (1, i) : buffer.getSample (0, i)) * (1.0f - mix) + wr[i] * mix;
        buffer.setSample (0, i, juce::jlimit (-1.2f, 1.2f, outL));
        if (outChannels > 1) buffer.setSample (1, i, juce::jlimit (-1.2f, 1.2f, outR));
    }
}

void VoxChoirAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void VoxChoirAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VoxChoirAudioProcessor::createEditor()
{
    return new VoxChoirAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxChoirAudioProcessor();
}
