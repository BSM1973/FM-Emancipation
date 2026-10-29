#pragma once
#include <JuceHeader.h>
#include <atomic>
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
    int getNumPrograms() override { return 100; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    float getLfoPhaseForDisplay(int index) const noexcept
    {
        return lfoDisplayPhases[(size_t)juce::jlimit(0, 1, index)].load(std::memory_order_relaxed);
    }
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    struct Voice {
        int note = -1;
        int midiChannel = 1;
        double phase[6] {};
        double feedbackSample[6] {};
        std::array<double, 2> noteLfoPhases {}; // independent modulation per MIDI voice
        // Two independent note-triggered modulation ADSRs (not operator envelopes).
        std::array<double, 2> modulationEnvelope {};
        std::array<double, 2> modulationReleaseStart {};
        std::array<int, 2> modulationStage {}; // 0=A 1=D 2=S 3=R 4=finished
        double operatorEnvelope[6] {};
        int operatorStage[6] {};
        double envelope = 0.0;
        double velocity = 0.0;
        bool releasing = false;
    };
    std::array<Voice, 16> voices {};
    double rate = 44100.0;
    std::array<double, 2> lfoPhases {}; // LFO 1 and LFO 2 run independently
    std::array<std::atomic<float>, 2> lfoDisplayPhases {}; // audio-to-UI snapshots, lock-free
    std::array<double, 16> pitchBendSemitones {};
    int currentProgram = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMEmancipationAudioProcessor)
};
