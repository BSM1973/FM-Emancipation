#pragma once
#include <JuceHeader.h>
class FMEmancipationAudioProcessor final : public juce::AudioProcessor
{
public:
    FMEmancipationAudioProcessor();
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "FM Emancipation"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Custom Sound"; }
    void changeProgramName(int, const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    struct Voice {
        int note = -1;
        double phase[6] {};
        double operatorEnvelope[6] {};
        int operatorStage[6] {};
        double envelope = 0.0;
        double velocity = 0.0;
        bool releasing = false;
    };
    std::array<Voice, 16> voices {};
    double rate = 44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMEmancipationAudioProcessor)
};
