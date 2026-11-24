#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>
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
    double getTailLengthSeconds() const override;
    int getNumPrograms() override { return 100; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    // MIDI CC bindings use stable parameter IDs in saved state.
    void armMidiLearn(const juce::String& id);
    void clearMidiLearn(const juce::String& id);
    int midiBinding(const juce::String& id) const;
    int midiLearnTarget() const noexcept { return learnTarget.load(); }
    std::atomic<bool> english { false };
    std::array<std::atomic<bool>, 100> favourites {};
    inline static constexpr int routes[20][6] = {
        {-1,0,1,2,3,4}, {-1,0,0,2,2,4},
        {-1,0,1,-1,3,4}, {-1,0,-1,2,3,4},
        {-1,0,0,0,3,4}, {-1,0,1,1,1,4},
        {-1,0,-1,2,-1,4}, {-1,0,1,-1,3,-1},
        {-1,0,-1,2,2,-1}, {-1,0,0,-1,3,-1},
        {-1,-1,1,2,3,4}, {-1,-1,1,-1,3,4},
        {-1,-1,1,1,-1,4}, {-1,-1,1,-1,3,-1},
        {-1,-1,-1,2,3,4}, {-1,-1,-1,2,-1,4},
        {-1,-1,-1,2,2,-1}, {-1,-1,-1,-1,3,4},
        {-1,-1,-1,-1,3,-1}, {-1,-1,-1,-1,-1,-1}
    };
    // 1-10: FM, 11-15: mixed FM/AM, 16-19: AM, 20: additive.
    static int routeType(int algorithmIndex, int op) noexcept {
        if (routes[algorithmIndex][op] < 0) return 0;
        if (algorithmIndex >= 15) return 2;
        if (algorithmIndex >= 10 && op % 2 == 1) return 3;
        return 1;
    }
    int getArpNoteForDisplay() const noexcept
    {
        return arpDisplayNote.load(std::memory_order_relaxed);
    }
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
        // Two zero-delay-feedback state-variable filter stages per MIDI voice.
        // Keeping integrators per voice avoids shared-state interference between notes.
        struct FilterIntegrator { double ic1eq = 0.0, ic2eq = 0.0; };
        std::array<FilterIntegrator, 2> filterIntegrators {};
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
    // Held keys are scoped per MIDI channel to preserve MPE/multichannel MIDI.
    // Values are input velocities, with zero indicating a released key.
    std::array<std::array<float, 128>, 16> arpHeldVelocities {};
    struct ArpNote { int pitch = -1, channel = 1; float velocity = 0.0f; };
    std::array<ArpNote, 16> arpPlaying {};
    int arpPlayingCount = 0;
    double arpStepCountdown = 0.0, arpGateCountdown = 0.0;
    unsigned int arpStepCounter = 0, arpRandomState = 0x7a9e3d21u;
    bool arpWasEnabled = false;
    std::atomic<int> arpDisplayNote { -1 };
    double rate = 44100.0;
    // Global stereo post-synthesis chain, prepared once; no heap allocation
    // in processBlock, and no effect changes the six-operator voice engine.
    juce::dsp::Chorus<float> chorusEffect;
    juce::Reverb reverbEffect;
    std::array<std::vector<float>, 2> delayMemory;
    int delayWriteIndex = 0;
    bool chorusWasEnabled = false, delayWasEnabled = false, reverbWasEnabled = false;
    juce::SmoothedValue<float> delayTimeSmoother, delayFeedbackSmoother, delayMixSmoother;
    juce::SmoothedValue<float> chorusDepthSmoother, reverbWetSmoother;
    std::atomic<double> tailHostBpm { 120.0 };
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative> cutoffSmoother;
    juce::SmoothedValue<double> resonanceSmoother, filterMixSmoother;
    int lastFilterMode = 0;
    std::array<double, 2> lfoPhases {}; // LFO 1 and LFO 2 run independently
    std::array<std::atomic<float>, 2> lfoDisplayPhases {}; // audio-to-UI snapshots, lock-free
    std::array<double, 16> pitchBendSemitones {};
    std::array<std::atomic<int>, 256> midiBindings {};
    std::atomic<int> learnTarget { -1 };
    void applyMidiCC(const juce::MidiMessage&);
    int currentProgram = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMEmancipationAudioProcessor)
};
