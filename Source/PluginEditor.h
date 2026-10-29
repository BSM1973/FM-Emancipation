#pragma once
#include "PluginProcessor.h"
class FMEmancipationAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit FMEmancipationAudioProcessorEditor(FMEmancipationAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    FMEmancipationAudioProcessor& processor;
    juce::Slider algorithm, fmDepth, amDepth, output;
    std::array<juce::Slider, 6> ratios, levels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMEmancipationAudioProcessorEditor)
};
