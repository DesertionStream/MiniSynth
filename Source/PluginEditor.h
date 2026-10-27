#pragma once
#include "PluginProcessor.h"

// Editor: preset selector on top, then every parameter as a native control, grouped in tabs.
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
    void addSlider (int page, const char* id, const juce::String& text, bool intStyle = false);
    void addCombo (int page, const char* id, const juce::String& text);
    void addToggle (int page, const char* id, const juce::String& text);
    void showPage (int p);

    MiniSynthProcessor& proc;
    juce::ComboBox presetBox;
    std::vector<juce::TextButton*> tabs;
    juce::OwnedArray<juce::TextButton> tabButtons;
    std::vector<std::unique_ptr<Row>> rows[3];
    int page = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniSynthEditor)
};
