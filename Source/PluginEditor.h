#pragma once
#include "PluginProcessor.h"
class FMEmancipationAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit FMEmancipationAudioProcessorEditor(FMEmancipationAudioProcessor&);
    ~FMEmancipationAudioProcessorEditor() override;
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    class Surface;
    std::unique_ptr<Surface> surface;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMEmancipationAudioProcessorEditor)
};
