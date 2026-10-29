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
    juce::Slider lfoRate, lfoDepth;
    juce::TextButton lfoMatrixButton { "MATRICE" };
    juce::ComboBox lfoWaveform, lfoSync, lfoDivision, lfoDestination;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> lfoComboAttachments;
    juce::ComboBox presetSelector;
    juce::TextButton savePresetButton { "ENREGISTRER" };
    juce::TextButton loadPresetButton { "CHARGER" };
    juce::ComboBox userPresetSelector;
    juce::TextEditor presetSearch;
    juce::TextButton clearPresetSearch { "EFFACER" };
    juce::ComboBox presetCategory; 
    juce::Array<juce::File> filteredPresetFiles;
    void refreshUserPresets();
    juce::Array<juce::File> userPresetFiles;
    std::array<juce::Slider, 6> ratios, levels;
    std::array<juce::Slider, 6> feedbackControls, velocityControls;
    std::array<juce::ComboBox, 6> waveformControls;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> waveformAttachments;
    std::array<juce::Slider, 6> opAttacks, opDecays, opSustains, opReleases;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMEmancipationAudioProcessorEditor)
};
