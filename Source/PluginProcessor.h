#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "PresetManager.h"
#include "DSP/DegradeEngine.h"
#include "DSP/ReverbEngine.h"

class BlgAudioProcessor : public juce::AudioProcessor
{
public:
    BlgAudioProcessor();
    ~BlgAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }   // reverb tail

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    blg::PresetManager presets;

private:
    blg::dsp::DegradeEngine degrade;
    blg::dsp::ReverbEngine  reverb;
    juce::SmoothedValue<float> stereoSmoothed;

    std::atomic<float>* pSampleRate = nullptr;
    std::atomic<float>* pBits       = nullptr;
    std::atomic<float>* pTurbo      = nullptr;
    std::atomic<float>* pTilt       = nullptr;
    std::atomic<float>* pRevDry     = nullptr;
    std::atomic<float>* pRevSize    = nullptr;
    std::atomic<float>* pStereo     = nullptr;
    std::atomic<float>* pRevWidth   = nullptr;
    std::atomic<float>* pRevDepth   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlgAudioProcessor)
};
