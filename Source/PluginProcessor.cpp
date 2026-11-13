#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float maxDriveDb   = 24.0f;   // pre-saturation gain at drive = 1
    constexpr float maxBassDb    = 9.0f;    // low shelf boost at bass = 1
    constexpr float shelfHz      = 90.0f;
    constexpr float compDb       = 8.0f;    // output trim at drive = 1, keeps loudness roughly even
    constexpr float satBias      = 0.12f;   // small asymmetry -> a bit of even harmonics (warmth)
}

MelluratorProcessor::MelluratorProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    driveParam = apvts.getRawParameterValue ("drive");
    bassParam  = apvts.getRawParameterValue ("bass");
}

juce::AudioProcessorValueTreeState::ParameterLayout MelluratorProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive",
                 NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.30f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "bass", 1 }, "Bass",
                 NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
    return { p.begin(), p.end() };
}

bool MelluratorProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void MelluratorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    lowShelf.prepare (spec);
    dcBlock.prepare (spec);
    *dcBlock.state = *Coeffs::makeHighPass (sampleRate, 18.0f);
    lastBassDb = -1000.0f;

    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);
    setLatencySamples ((int) std::ceil (oversampling.getLatencyInSamples()));

    driveSmoothed.reset (sampleRate * (double) (1 << oversamplingOrder), 0.03);
    driveSmoothed.setCurrentAndTargetValue (driveParam->load());
}

void MelluratorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const float drive = driveParam->load();
    const float bass  = bassParam->load();
    driveForUI.store (drive);

    // 1) low boost, before the saturator so the boosted lows get harmonics too
    const float bassDb = bass * maxBassDb;
    if (std::abs (bassDb - lastBassDb) > 0.01f)
    {
        *lowShelf.state = *Coeffs::makeLowShelf (currentSampleRate, shelfHz, 0.707f,
                                                 juce::Decibels::decibelsToGain (bassDb));
        lastBassDb = bassDb;
    }

    juce::dsp::AudioBlock<float> block (buffer);
    auto stereo = block.getSubsetChannelBlock (0, (size_t) numCh);
    lowShelf.process (juce::dsp::ProcessContextReplacing<float> (stereo));

    // 2) saturation at 4x oversampling
    driveSmoothed.setTargetValue (drive);
    auto up = oversampling.processSamplesUp (stereo);
    const size_t n = up.getNumSamples();

    for (size_t i = 0; i < n; ++i)
    {
        const float d    = driveSmoothed.getNextValue();
        const float gain = juce::Decibels::decibelsToGain (d * maxDriveDb);
        const float comp = juce::Decibels::decibelsToGain (-d * compDb);
        const float dc   = std::tanh (satBias);

        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            auto* x = up.getChannelPointer (ch);
            x[i] = (std::tanh (x[i] * gain + satBias) - dc) * comp;
        }
    }

    oversampling.processSamplesDown (stereo);

    // 3) remove the DC the bias introduced
    dcBlock.process (juce::dsp::ProcessContextReplacing<float> (stereo));
}

juce::AudioProcessorEditor* MelluratorProcessor::createEditor()
{
    return new MelluratorEditor (*this);
}

void MelluratorProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void MelluratorProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MelluratorProcessor();
}
