#pragma once
#include "PluginProcessor.h"
class FMEmancipationAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit FMEmancipationAudioProcessorEditor(FMEmancipationAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    // Five real pages, plus contextual sub-navigation within complex modules.
    void selectTab(int index);
    void updateTabVisibility();
    int currentTab = 0;
    int currentEnvelopePage = 0;
    int currentEffectPage = 0;
    int currentAlgorithmPage = 0;
    std::array<juce::TextButton, 5> navigationTabs;
    std::array<juce::TextButton, 2> envelopeTabs, effectTabs, algorithmTabs;
    // Existing attached JUCE module editors become embedded page components.
    std::unique_ptr<juce::Component> studioEffectsPage, multimodeFilterPage;
    std::unique_ptr<juce::Component> modulationEnvelopePage, arpeggiatorPage;
    FMEmancipationAudioProcessor& processor;
    juce::Slider algorithm, fmDepth, amDepth, output;
    juce::Slider lfoRate, lfoDepth, lfo2Rate, lfo2Depth;
    juce::ComboBox lfoSourceSelector;
    void updateLfoSourceVisibility();
    juce::TextButton lfoMatrixButton { "MATRICE" };
    juce::TextButton lfoPhaseModeButton { "PHASE / MODE" };
    juce::TextButton modulationEnvelopesButton { "ENV MOD 1/2" };
    juce::TextButton filterButton { "FILTRE" };
    juce::TextButton effectsButton { "EFFETS" };
    juce::TextButton arpeggiatorButton { "ARPÉGIATEUR" };
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
