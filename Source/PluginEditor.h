#pragma once
#include "PluginProcessor.h"
class FMEmancipationAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit FMEmancipationAudioProcessorEditor(FMEmancipationAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    FMEmancipationAudioProcessor& processor;
    juce::Slider algorithm, fmDepth, amDepth, output;
    juce::Slider lfoRate, lfoDepth, lfo2Rate, lfo2Depth;
    juce::ComboBox lfoSourceSelector;
    void updateLfoSourceVisibility();
    juce::TextButton lfoMatrixButton { "MATRICE" };
    juce::ComboBox lfoWaveform, lfoSync, lfoDivision, lfoDestination;
    juce::ComboBox lfo2Waveform, lfo2Sync, lfo2Division, lfo2Destination;
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
