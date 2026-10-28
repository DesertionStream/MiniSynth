#pragma once
#include "PluginProcessor.h"

// Piano roll: 16 steps (one bar of 16th notes) x 37 semitone rows around the chord root.
class PianoRoll : public juce::Component, private juce::Timer
{
public:
    explicit PianoRoll (MiniSynthProcessor& p) : proc (p) { startTimerHz (30); }
    void setPreviewChord (int c) { chord = c; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    void timerCallback() override { repaint(); }
    void apply (const juce::MouseEvent&);
    bool cellAt (juce::Point<int>, int& step, int& row) const;
    MiniSynthProcessor& proc;
    int chord = 0; bool paintOn = true; int lastStep = -1, lastRow = -1;
    static constexpr int labelW = 52;
};

class MiniSynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit MiniSynthEditor (MiniSynthProcessor&);
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    struct Row { std::unique_ptr<juce::Label> label; std::unique_ptr<juce::Component> ctl;
                 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa;
                 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca;
                 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba; };
    void addSlider (int page, const char* id, const juce::String& text);
    void addCombo (int page, const char* id, const juce::String& text);
    void addToggle (int page, const char* id, const juce::String& text);
    void showPage (int p);

    MiniSynthProcessor& proc;
    juce::ComboBox presetBox;
    juce::TextButton saveBtn { "Sauver..." }, loadBtn { "Charger..." }, defBtn { "Definir par defaut" };
    juce::OwnedArray<juce::TextButton> tabButtons;
    std::vector<std::unique_ptr<Row>> rows[4];
    PianoRoll roll;
    juce::ComboBox rollChord; juce::TextButton rollClear { "Effacer" }, rollFill { "Remplir depuis le motif auto" };
    juce::Label rollHint;
    std::unique_ptr<juce::FileChooser> chooser;
    int page = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniSynthEditor)
};
