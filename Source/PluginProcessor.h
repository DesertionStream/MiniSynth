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
    int getCurrentProgram() override { return 0; }
    int getCurrentPreset() const { return currentProgram; }
    void setCurrentProgram (int i) override;
    int getNumPresets(); const juce::String getPresetName (int i); void applyPreset (int i);
    const juce::String getProgramName (int i) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // piano-roll pattern: one 64-bit mask per 16th step; bit r = semitone offset (r - 12) from the chord root
    static constexpr int rollRows = 37;
    std::array<std::atomic<juce::uint64>, 16> roll;
    std::atomic<int> currentStep { -1 };
    void setRollStep (int step, juce::uint64 mask);
    void fillRollFromAuto (int chord);
    void clearRoll();
    bool saveStateTo (const juce::File&);
    bool loadStateFrom (const juce::File&);
    void saveAsDefault();
    static juce::File settingsDir();

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void readParams();
    void triggerNote (int midi, float vel, bool arp, int arpSamples);
    void arpStep (juce::int64 stepIdx);
    void releaseArpVoices();
    void syncRollToState();
    void loadRollFromState();
    static int buildNotes (int root, const std::vector<int>& iv, int octs, int* out, int cap);
    static int patternIdx (int kind, int len, juce::int64 step, juce::Random& r);
    std::vector<int> heldMidi;
    int arpSrc = 0;

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
