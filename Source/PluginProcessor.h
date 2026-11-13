#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class MelluratorProcessor : public juce::AudioProcessor
{
public:
    MelluratorProcessor();
    ~MelluratorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mellurator"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // 0..1, read by the editor to drive pixelation. Written on the audio thread.
    std::atomic<float> driveForUI { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static constexpr int oversamplingOrder = 2;   // 2^2 = 4x
    juce::dsp::Oversampling<float> oversampling {
        2, oversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    juce::dsp::ProcessorDuplicator<Filter, Coeffs> lowShelf, dcBlock;

    juce::SmoothedValue<float> driveSmoothed;   // runs at the oversampled rate
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* bassParam  = nullptr;
    double currentSampleRate = 44100.0;
    float lastBassDb = -1000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MelluratorProcessor)
};
