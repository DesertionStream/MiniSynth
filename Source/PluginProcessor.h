#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Synth.h"

class MiniSynthProcessor : public juce::AudioProcessor
{
public:
    MiniSynthProcessor();
    ~MiniSynthProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Mini Synthe"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int i) override;
    const juce::String getProgramName (int i) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void readParams();
    void triggerNote (int midi, float vel, bool arp, int arpSamples);
    void arpStep (juce::int64 stepIdx);
    void releaseArpVoices();

    static constexpr int numVoices = 24;
    std::array<ms::Voice, numVoices> voices;
    ms::Params P;
    ms::BlockCtx bc;
    ms::Effects fx;
    juce::Random rnd;
    juce::uint64 ageCounter = 0;
    float lastFreq = 0.f;
    double sr = 44100.0;

    // arpeggiator
    bool arpOn = true, arpSync = true, arpWasActive = false;
    int arpPattern = 0, arpOcts = 2, arpBars = 2;
    float arpGate = 1.1f;
    int roots[4] {9,9,7,5}, modeIdx[4] {1,1,0,0};
    double bpmNow = 158.0, samplesPerStep = 0, arpCountdown = 0;
    juce::int64 arpStepIdx = 0;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniSynthProcessor)
};
