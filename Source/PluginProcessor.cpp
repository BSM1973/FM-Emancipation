#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
constexpr double twoPi = juce::MathConstants<double>::twoPi;

// Topology-preserving 2-pole state-variable filter (TPT/SVF).
struct FilterSignals { double low, high, band, notch; };
FilterSignals processSvf(double input, double g, double k,
                         double& ic1eq, double& ic2eq) noexcept
{
    const double a1 = 1.0 / (1.0 + g * (g + k));
    const double v1 = a1 * (ic1eq + g * (input - ic2eq));
    const double v2 = ic2eq + g * v1;
    ic1eq = 2.0 * v1 - ic1eq;
    ic2eq = 2.0 * v2 - ic2eq;
    const double high = input - k * v1 - v2;
    return { v2, high, k * v1, high + v2 };
}

double evaluateLfo(double phase, int shape) noexcept
{
    const double fraction = (phase - twoPi * std::floor(phase / twoPi)) / twoPi;
    if (shape == 1) return 1.0 - 4.0 * std::abs(fraction - 0.5);
    if (shape == 2) return 2.0 * fraction - 1.0;
    if (shape == 3) return fraction < 0.5 ? 1.0 : -1.0;
    return std::sin(phase);
}
}
juce::AudioProcessorValueTreeState::ParameterLayout FMEmancipationAudioProcessor::makeParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto f = [&](juce::String id, juce::String label, float lo, float hi, float def) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 1 }, label,
            juce::NormalisableRange<float>(lo, hi), def));
    };
    f("algorithm", "Algorithm", 1, 20, 1);
    f("fmDepth", "FM depth", 0, 10, 3);
    f("amDepth", "AM depth", 0, 1, 0.65f);
    f("output", "Output gain", 0, 1, 0.35f);
    f("attack", "Attack", 0.001f, 5, 0.01f);
    f("release", "Release", 0.01f, 8, 0.35f);
    f("filterMode", "Filter type", 0.0f, 6.0f, 0.0f);
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "filterCutoff", 1 }, "Filter cutoff (Hz)",
        juce::NormalisableRange<float>(25.0f, 18000.0f, 0.0f, 0.27f), 9000.0f));
    f("filterResonance", "Filter resonance", 0.0f, 1.0f, 0.15f);
    f("filterMix", "Filter wet/dry mix", 0.0f, 1.0f, 1.0f);
    f("filterEnv1", "ENV 1 to filter cutoff", -1.0f, 1.0f, 0.0f);
    f("filterEnv2", "ENV 2 to filter cutoff", -1.0f, 1.0f, 0.0f);
    // Bipolar independent LFO routings; zero keeps existing presets identical.
    f("filterLfo1Cutoff", "LFO 1 to filter cutoff", -1.0f, 1.0f, 0.0f);
    f("filterLfo2Cutoff", "LFO 2 to filter cutoff", -1.0f, 1.0f, 0.0f);
    f("filterLfo1Resonance", "LFO 1 to filter resonance", -1.0f, 1.0f, 0.0f);
    f("filterLfo2Resonance", "LFO 2 to filter resonance", -1.0f, 1.0f, 0.0f);
    // The LFO defaults to OFF and does not alter existing sessions.
    f("lfoWave", "LFO waveform", 0.0f, 3.0f, 0.0f);
    f("lfoRate", "LFO free rate (Hz)", 0.05f, 20.0f, 5.0f);
    f("lfoSync", "LFO tempo synchronization", 0.0f, 1.0f, 0.0f);
    f("lfoDivision", "LFO beat division", 0.0f, 6.0f, 2.0f);
    f("lfoDepth", "LFO modulation amount", 0.0f, 1.0f, 0.0f);
    f("lfoTarget", "LFO destination", 0.0f, 5.0f, 0.0f);
    // Second independent modulation oscillator; OFF for existing presets.
    f("lfo2Wave", "LFO 2 waveform", 0.0f, 3.0f, 0.0f);
    f("lfo2Rate", "LFO 2 free rate (Hz)", 0.05f, 20.0f, 1.0f);
    f("lfo2Sync", "LFO 2 tempo synchronization", 0.0f, 1.0f, 0.0f);
    f("lfo2Division", "LFO 2 beat division", 0.0f, 6.0f, 2.0f);
    f("lfo2Depth", "LFO 2 modulation amount", 0.0f, 1.0f, 0.0f);
    f("lfo2Target", "LFO 2 destination", 0.0f, 5.0f, 0.0f);
    f("lfoPhaseOffset", "LFO 1 initial phase", 0.0f, 1.0f, 0.0f);
    f("lfo2PhaseOffset", "LFO 2 initial phase", 0.0f, 1.0f, 0.0f);
    f("lfoRetrigger", "LFO 1 note retrigger", 0.0f, 1.0f, 0.0f);
    f("lfo2Retrigger", "LFO 2 note retrigger", 0.0f, 1.0f, 0.0f);
    // Two additional ADSRs are strictly modulation sources, independent of
    // the six oscillator envelopes. Depth zero and target OFF preserve all presets.
    for (int i = 0; i < 2; ++i) {
        const auto base = "modEnv" + juce::String(i + 1);
        f(base + "Attack", "ENV " + juce::String(i + 1) + " attack", 0.001f, 8.0f, 0.01f);
        f(base + "Decay", "ENV " + juce::String(i + 1) + " decay", 0.001f, 8.0f, 0.35f);
        f(base + "Sustain", "ENV " + juce::String(i + 1) + " sustain", 0.0f, 1.0f, 0.7f);
        f(base + "Release", "ENV " + juce::String(i + 1) + " release", 0.005f, 12.0f, 0.4f);
        f(base + "Amount", "ENV " + juce::String(i + 1) + " modulation amount", -1.0f, 1.0f, 0.0f);
        f(base + "Target", "ENV " + juce::String(i + 1) + " modulation destination", 0.0f, 5.0f, 0.0f);
    }
    for (int i = 0; i < 6; ++i) {
        f("ratio" + juce::String(i), "OP " + juce::String(i + 1) + " ratio", 0.25f, 16, i == 2 ? 3.0f : 1.0f);
        f("level" + juce::String(i), "OP " + juce::String(i + 1) + " level", 0, 1, i < 2 ? 0.8f : 0.5f);
        f("opAttack" + juce::String(i), "OP " + juce::String(i + 1) + " attack", 0.001f, 5.0f, 0.01f);
        f("opDecay" + juce::String(i), "OP " + juce::String(i + 1) + " decay", 0.001f, 8.0f, 0.25f);
        f("opSustain" + juce::String(i), "OP " + juce::String(i + 1) + " sustain", 0.0f, 1.0f, 1.0f);
        f("opRelease" + juce::String(i), "OP " + juce::String(i + 1) + " release", 0.01f, 8.0f, 0.35f);
    }
    for (int i = 0; i < 6; ++i) {
        f("feedback" + juce::String(i), "Operator feedback", 0.0f, 1.0f, 0.0f);
        f("wave" + juce::String(i), "Operator waveform", 0.0f, 3.0f, 0.0f);
        f("velocity" + juce::String(i), "Operator velocity sensitivity", 0.0f, 1.0f, 0.0f);
    }
    // Bipolar per-operator LFO modulation matrix. Zero means no modulation.
    // The defaults preserve all existing presets and sessions.
    for (int i = 0; i < 6; ++i) {
        const auto op = juce::String(i);
        f("lfoOpPitch" + op, "OP " + juce::String(i + 1) + " LFO pitch", -1.0f, 1.0f, 0.0f);
        f("lfoOpLevel" + op, "OP " + juce::String(i + 1) + " LFO level", -1.0f, 1.0f, 0.0f);
        f("lfoOpFM" + op, "OP " + juce::String(i + 1) + " LFO FM", -1.0f, 1.0f, 0.0f);
        f("lfoOpFeedback" + op, "OP " + juce::String(i + 1) + " LFO feedback", -1.0f, 1.0f, 0.0f);
        f("lfo2OpPitch" + op, "OP " + juce::String(i + 1) + " LFO 2 pitch", -1.0f, 1.0f, 0.0f);
        f("lfo2OpLevel" + op, "OP " + juce::String(i + 1) + " LFO 2 level", -1.0f, 1.0f, 0.0f);
        f("lfo2OpFM" + op, "OP " + juce::String(i + 1) + " LFO 2 FM", -1.0f, 1.0f, 0.0f);
        f("lfo2OpFeedback" + op, "OP " + juce::String(i + 1) + " LFO 2 feedback", -1.0f, 1.0f, 0.0f);
    }
    return { p.begin(), p.end() };
}
FMEmancipationAudioProcessor::FMEmancipationAudioProcessor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "FMEmancipationState", makeParameters()) {}
bool FMEmancipationAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}
void FMEmancipationAudioProcessor::prepareToPlay(double sr, int)
{
    rate = sr > 0 ? sr : 44100.0;
    voices = {};
    pitchBendSemitones.fill(0.0);
    lfoPhases.fill(0.0);
    for (auto& phase : lfoDisplayPhases) phase.store(0.0f, std::memory_order_relaxed);
    cutoffSmoother.reset(rate, 0.025);
    resonanceSmoother.reset(rate, 0.025);
    filterMixSmoother.reset(rate, 0.025);
    cutoffSmoother.setCurrentAndTargetValue(
        parameters.getRawParameterValue("filterCutoff")->load());
    resonanceSmoother.setCurrentAndTargetValue(
        parameters.getRawParameterValue("filterResonance")->load());
    filterMixSmoother.setCurrentAndTargetValue(
        parameters.getRawParameterValue("filterMix")->load());
    lastFilterMode = 0;
}
void FMEmancipationAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    auto param = [&](const juce::String& key) { return parameters.getRawParameterValue(key)->load(); };
    const int algorithm = juce::jlimit(1, 20, (int)std::round(param("algorithm")));
    const double depth = param("fmDepth");
    const double am = param("amDepth");
    const double output = param("output");
    const double attack = param("attack");
    const double release = param("release");
    const int filterMode = juce::jlimit(0, 6, (int)std::lround(param("filterMode")));
    cutoffSmoother.setTargetValue(param("filterCutoff"));
    resonanceSmoother.setTargetValue(param("filterResonance"));
    filterMixSmoother.setTargetValue(param("filterMix"));
    const double filterEnvDepth[2] { param("filterEnv1"), param("filterEnv2") };
    const double filterLfoCutoff[2] {
        param("filterLfo1Cutoff"), param("filterLfo2Cutoff")
    };
    const double filterLfoResonance[2] {
        param("filterLfo1Resonance"), param("filterLfo2Resonance")
    };
    if (filterMode != lastFilterMode) {
        // Clear integrators only on topology changes, not on every note/block.
        for (auto& voice : voices)
            for (auto& stage : voice.filterIntegrators) stage = {};
        lastFilterMode = filterMode;
    }
    const int lfoWave = juce::jlimit(0, 3, (int)std::round(param("lfoWave")));
    const double lfoRate = param("lfoRate");
    const bool lfoSync = param("lfoSync") >= 0.5f;
    const int lfoDivision = juce::jlimit(0, 6, (int)std::round(param("lfoDivision")));
    const double lfoDepth = param("lfoDepth");
    const int lfoTarget = juce::jlimit(0, 5, (int)std::round(param("lfoTarget")));
    const int lfo2Wave = juce::jlimit(0, 3, (int)std::round(param("lfo2Wave")));
    const double lfo2Rate = param("lfo2Rate");
    const bool lfo2Sync = param("lfo2Sync") >= 0.5f;
    const int lfo2Division = juce::jlimit(0, 6, (int)std::round(param("lfo2Division")));
    const double lfo2Depth = param("lfo2Depth");
    const int lfo2Target = juce::jlimit(0, 5, (int)std::round(param("lfo2Target")));
    const std::array<double, 2> lfoStartOffsets {
        twoPi * param("lfoPhaseOffset"), twoPi * param("lfo2PhaseOffset")
    };
    const std::array<bool, 2> noteRetrigger {
        param("lfoRetrigger") >= 0.5f, param("lfo2Retrigger") >= 0.5f
    };
    // Snapshot automation once per block; no string lookups or allocation
    // in the polyphonic per-sample processing loop.
    double envAttack[2], envDecay[2], envSustain[2], envRelease[2], envAmount[2];
    int envTarget[2];
    for (int i = 0; i < 2; ++i) {
        const auto base = "modEnv" + juce::String(i + 1);
        envAttack[i] = param(base + "Attack");
        envDecay[i] = param(base + "Decay");
        envSustain[i] = param(base + "Sustain");
        envRelease[i] = param(base + "Release");
        envAmount[i] = param(base + "Amount");
        envTarget[i] = juce::jlimit(0, 5, (int)std::lround(param(base + "Target")));
    }

    // Quarter-note units per complete cycle: 1 bar, 1/2, 1/4,
    // 1/8, 1/16, dotted 1/8, and triplet 1/8.
    static constexpr double lfoBeatsPerCycle[] = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.75, 1.0 / 3.0 };
    double hostBpm = 120.0;
    double hostPpq = 0.0;
    bool hasHostPpq = false;
    bool transportPlaying = true;
    if (auto* playHead = getPlayHead()) {
        if (const auto position = playHead->getPosition()) {
            if (const auto bpm = position->getBpm())
                if (std::isfinite(*bpm) && *bpm > 0.0) hostBpm = *bpm;
            if (const auto ppq = position->getPpqPosition()) {
                if (std::isfinite(*ppq)) {
                    hostPpq = *ppq;
                    hasHostPpq = true;
                }
            }
            transportPlaying = position->getIsPlaying();
        }
    }

    // When the host exposes musical time, lock each synced LFO to the project
    // timeline at the beginning of every block. Seeking and arrangement loops
    // thus relocate the LFO phase, without affecting free-running oscillators.
    const auto lockToHost = [&](int index, bool synced, int division) {
        if (!synced || !hasHostPpq) return;
        double cycles = std::fmod(hostPpq / lfoBeatsPerCycle[division], 1.0);
        if (cycles < 0.0) cycles += 1.0;
        lfoPhases[(size_t)index] = twoPi * cycles;
    };
    lockToHost(0, lfoSync, lfoDivision);
    lockToHost(1, lfo2Sync, lfo2Division);

    const double lfoHz = lfoSync ? hostBpm / (60.0 * lfoBeatsPerCycle[lfoDivision]) : lfoRate;
    const double lfo2Hz = lfo2Sync ? hostBpm / (60.0 * lfoBeatsPerCycle[lfo2Division]) : lfo2Rate;
    const auto phaseIncrement = [&](double hz, bool synced) {
        if (synced && hasHostPpq && !transportPlaying) return 0.0;
        return twoPi * juce::jlimit(0.0, rate * 0.45, hz) / rate;
    };
    const double lfoPhaseStep = phaseIncrement(lfoHz, lfoSync);
    const double lfo2PhaseStep = phaseIncrement(lfo2Hz, lfo2Sync);
    double ratios[6], levels[6], opAttack[6], opDecay[6], opSustain[6], opRelease[6];
    for (int i = 0; i < 6; ++i) {
        ratios[i] = param("ratio" + juce::String(i));
        levels[i] = param("level" + juce::String(i));
        opAttack[i] = param("opAttack" + juce::String(i));
        opDecay[i] = param("opDecay" + juce::String(i));
        opSustain[i] = param("opSustain" + juce::String(i));
        opRelease[i] = param("opRelease" + juce::String(i));
    }
    double feedback[6], velocitySensitivity[6];
    int waveType[6];
    for (int i = 0; i < 6; ++i) {
        feedback[i] = param("feedback" + juce::String(i));
        velocitySensitivity[i] = param("velocity" + juce::String(i));
        waveType[i] = juce::jlimit(0, 3, (int)std::round(param("wave" + juce::String(i))));
    }
    double matrixPitch[6], matrixLevel[6], matrixFM[6], matrixFeedback[6];
    double matrix2Pitch[6], matrix2Level[6], matrix2FM[6], matrix2Feedback[6];
    for (int i = 0; i < 6; ++i) {
        const auto op = juce::String(i);
        matrixPitch[i] = param("lfoOpPitch" + op);
        matrixLevel[i] = param("lfoOpLevel" + op);
        matrixFM[i] = param("lfoOpFM" + op);
        matrixFeedback[i] = param("lfoOpFeedback" + op);
        matrix2Pitch[i] = param("lfo2OpPitch" + op);
        matrix2Level[i] = param("lfo2OpLevel" + op);
        matrix2FM[i] = param("lfo2OpFM" + op);
        matrix2Feedback[i] = param("lfo2OpFeedback" + op);
    }
    juce::MidiBuffer::Iterator iterator(midi);
    juce::MidiMessage message;
    int eventSample = 0;
    bool hasEvent = iterator.getNextEvent(message, eventSample);
    for (int s = 0; s < buffer.getNumSamples(); ++s) {
        const double currentCutoff = cutoffSmoother.getNextValue();
        const double currentResonance = resonanceSmoother.getNextValue();
        const double currentFilterMix = filterMixSmoother.getNextValue();
        while (hasEvent && eventSample <= s) {
            if (message.isNoteOn()) {
                Voice* slot = nullptr;
                for (auto& v : voices) if (v.note < 0) { slot = &v; break; }
                if (!slot) slot = &voices[0];
                *slot = {};
                slot->note = message.getNoteNumber();
                slot->midiChannel = message.getChannel();
                slot->velocity = message.getFloatVelocity();
            } else if (message.isNoteOff()) {
                for (auto& v : voices)
                    if (v.note == message.getNoteNumber() && v.midiChannel == message.getChannel()) v.releasing = true;
            } else if (message.isPitchWheel()) {
                const int channel = juce::jlimit(1, 16, message.getChannel()) - 1;
                pitchBendSemitones[(size_t)channel] = (message.getPitchWheelValue() - 8192) * (2.0 / 8192.0);
            } else if (message.isAllNotesOff() || message.isAllSoundOff()) {
                for (auto& v : voices) v = {};
            }
            hasEvent = iterator.getNextEvent(message, eventSample);
        }
        // Global LFO phase runs continuously, including while notes overlap.
        // Per-note mode uses separate phases stored in each Voice instead.
        const double globalWave1 = evaluateLfo(lfoPhases[0] + lfoStartOffsets[0], lfoWave);
        const double globalWave2 = evaluateLfo(lfoPhases[1] + lfoStartOffsets[1], lfo2Wave);
        lfoPhases[0] += lfoPhaseStep;
        lfoPhases[1] += lfo2PhaseStep;
        for (auto& phase : lfoPhases)
            if (phase >= twoPi) phase -= twoPi * std::floor(phase / twoPi);

        const bool matrixEnabled = lfoTarget == 5 && lfoDepth > 0.0;
        const bool matrix2Enabled = lfo2Target == 5 && lfo2Depth > 0.0;
        double mix = 0.0;
        for (auto& v : voices) {
            if (v.note < 0) continue;
            const double step = 1.0 / (rate * (v.releasing ? release : attack));
            v.envelope = v.releasing ? juce::jmax(0.0, v.envelope - step)
                                     : juce::jmin(1.0, v.envelope + step);
            if (v.releasing && v.envelope <= 0) { v = {}; continue; }

            // Each modulation envelope belongs to this voice: triggering
            // another note cannot restart a held note's envelope.
            double modEnvelopeValues[2] {};
            double envelopePitchSemitones = 0.0;
            double envelopeFM = 1.0;
            double envelopeAM = 0.0;
            double envelopeLevel = 1.0;
            double envelopeFeedback = 0.0;
            for (int e = 0; e < 2; ++e) {
                auto& value = v.modulationEnvelope[(size_t)e];
                auto& stage = v.modulationStage[(size_t)e];
                if (v.releasing && stage < 3) {
                    v.modulationReleaseStart[(size_t)e] = value;
                    stage = 3;
                }
                if (stage == 0) {
                    value = juce::jmin(1.0, value + 1.0 / (rate * envAttack[e]));
                    if (value >= 1.0) stage = 1;
                } else if (stage == 1) {
                    const double floor = envSustain[e];
                    value = juce::jmax(floor, value - (1.0 - floor) / (rate * envDecay[e]));
                    if (value <= floor) stage = 2;
                } else if (stage == 2) {
                    value = envSustain[e];
                } else if (stage == 3) {
                    value = juce::jmax(0.0, value
                        - v.modulationReleaseStart[(size_t)e] / (rate * envRelease[e]));
                    if (value <= 0.0) stage = 4;
                } else {
                    value = 0.0;
                }

                modEnvelopeValues[e] = value;
                const double modulationValue = value * envAmount[e];
                switch (envTarget[e]) {
                    case 1: envelopePitchSemitones += 12.0 * modulationValue; break;
                    case 2: envelopeFM *= juce::jmax(0.0, 1.0 + modulationValue); break;
                    case 3: envelopeAM += 0.5 * modulationValue; break;
                    case 4: envelopeLevel *= juce::jmax(0.0, 1.0 + modulationValue); break;
                    case 5: envelopeFeedback += 0.35 * modulationValue; break;
                    default: break;
                }
            }

            // Note retrigger starts at the initial phase for every new note.
            // Voices progress independently, so chords do not reset held notes.
            const double wave1 = noteRetrigger[0]
                ? evaluateLfo(v.noteLfoPhases[0] + lfoStartOffsets[0], lfoWave)
                : globalWave1;
            const double wave2 = noteRetrigger[1]
                ? evaluateLfo(v.noteLfoPhases[1] + lfoStartOffsets[1], lfo2Wave)
                : globalWave2;
            if (noteRetrigger[0]) {
                v.noteLfoPhases[0] += lfoPhaseStep;
                if (v.noteLfoPhases[0] >= twoPi)
                    v.noteLfoPhases[0] -= twoPi * std::floor(v.noteLfoPhases[0] / twoPi);
            }
            if (noteRetrigger[1]) {
                v.noteLfoPhases[1] += lfo2PhaseStep;
                if (v.noteLfoPhases[1] >= twoPi)
                    v.noteLfoPhases[1] -= twoPi * std::floor(v.noteLfoPhases[1] / twoPi);
            }
            const double mod = wave1 * lfoDepth;
            const double mod2 = wave2 * lfo2Depth;
            const double pitchSemitones = envelopePitchSemitones
                                        + (lfoTarget == 1 ? 2.0 * mod : 0.0)
                                        + (lfo2Target == 1 ? 2.0 * mod2 : 0.0);
            const double pitchFactor = pitchSemitones != 0.0 ? std::pow(2.0, pitchSemitones / 12.0) : 1.0;
            const double tremolo = (lfoTarget == 2 ? 1.0 - 0.5 * lfoDepth + 0.5 * mod : 1.0)
                                 * (lfo2Target == 2 ? 1.0 - 0.5 * lfo2Depth + 0.5 * mod2 : 1.0);
            const double modulatedFMDepth = juce::jmax(0.0, depth * envelopeFM
                * (lfoTarget == 3 ? 1.0 + mod : 1.0)
                * (lfo2Target == 3 ? 1.0 + mod2 : 1.0));
            const double modulatedAMDepth = juce::jlimit(0.0, 1.0, am + envelopeAM
                + (lfoTarget == 4 ? 0.5 * mod : 0.0)
                + (lfo2Target == 4 ? 0.5 * mod2 : 0.0));
            const double bend = pitchBendSemitones[(size_t)(v.midiChannel - 1)];
            const double hz = juce::MidiMessage::getMidiNoteInHertz(v.note) * std::pow(2.0, bend / 12.0) * pitchFactor;
            double op[6] {};
            // Twenty explicit six-operator routings. Each row lists the
            // destination operator for OP1..OP6 (-1 means a carrier).
            // Modulation flows from higher operator indices to lower ones.
            static constexpr int routes[20][6] = {
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
            const auto& route = routes[algorithm - 1];
            double modulation[6] {};
            double carrierSum = 0.0;
            int carrierCount = 0;
            for (int i = 5; i >= 0; --i) {
                auto& env = v.operatorEnvelope[i];
                auto& stage = v.operatorStage[i];
                if (v.releasing && stage != 3) stage = 3;
                if (stage == 0) {
                    env = juce::jmin(1.0, env + 1.0 / (rate * opAttack[i]));
                    if (env >= 1.0) stage = 1;
                } else if (stage == 1) {
                    env = juce::jmax(opSustain[i], env - (1.0 - opSustain[i]) / (rate * opDecay[i]));
                    if (env <= opSustain[i]) stage = 2;
                } else if (stage == 2) {
                    env = opSustain[i];
                } else {
                    env = juce::jmax(0.0, env - 1.0 / (rate * opRelease[i]));
                }
                const double fmMatrixAmount = (matrixEnabled ? mod * matrixFM[i] : 0.0)
                                            + (matrix2Enabled ? mod2 * matrix2FM[i] : 0.0);
                const double opFMDepth = juce::jmax(0.0, modulatedFMDepth * (1.0 + fmMatrixAmount));
                const double opFeedbackAmount = juce::jlimit(0.0, 1.0, feedback[i]
                    + envelopeFeedback
                    + (matrixEnabled ? 0.5 * mod * matrixFeedback[i] : 0.0)
                    + (matrix2Enabled ? 0.5 * mod2 * matrix2Feedback[i] : 0.0));
                const double phase = v.phase[i] + modulation[i] * opFMDepth
                    + v.feedbackSample[i] * opFeedbackAmount * 4.0;
                const double unit = (phase - twoPi * std::floor(phase / twoPi)) / twoPi;
                double wave = std::sin(phase);
                if (waveType[i] == 1) wave = 1.0 - 4.0 * std::abs(unit - 0.5);
                else if (waveType[i] == 2) wave = 2.0 * unit - 1.0;
                else if (waveType[i] == 3) wave = unit < 0.5 ? 1.0 : -1.0;
                v.feedbackSample[i] = wave;
                const double velocityGain = 1.0 - velocitySensitivity[i] + velocitySensitivity[i] * v.velocity;
                const double levelBaseline = (matrixEnabled ? std::abs(matrixLevel[i]) * lfoDepth : 0.0)
                    + (matrix2Enabled ? std::abs(matrix2Level[i]) * lfo2Depth : 0.0);
                const double opLevelFactor = juce::jmax(0.0, 1.0 - 0.5 * levelBaseline
                    + (matrixEnabled ? 0.5 * mod * matrixLevel[i] : 0.0)
                    + (matrix2Enabled ? 0.5 * mod2 * matrix2Level[i] : 0.0));
                op[i] = wave * levels[i] * env * velocityGain * opLevelFactor;
                if (route[i] < 0) {
                    carrierSum += op[i];
                    ++carrierCount;
                } else {
                    modulation[route[i]] += op[i];
                }
                const double opPitchOffset = (matrixEnabled ? 2.0 * mod * matrixPitch[i] : 0.0)
                    + (matrix2Enabled ? 2.0 * mod2 * matrix2Pitch[i] : 0.0);
                const double opPitchFactor = opPitchOffset != 0.0 ? std::pow(2.0, opPitchOffset / 12.0) : 1.0;
                v.phase[i] += twoPi * hz * ratios[i] * opPitchFactor / rate;
                if (v.phase[i] >= twoPi) v.phase[i] = std::fmod(v.phase[i], twoPi);
            }
            const double sum = carrierSum / juce::jmax(1, carrierCount);
            const double amSource = 0.5 + 0.5 * op[5];
            double voiceSample = sum * ((1.0 - modulatedAMDepth) + modulatedAMDepth * amSource)
                * tremolo * envelopeLevel * v.envelope * v.velocity;

            if (filterMode != 0 && currentFilterMix > 0.0) {
                // Bipolar modulation from both independent ADSRs; positive
                // values open the cutoff as the envelope rises.
                // LFO-to-filter routes work independently of the global LFO
                // destinations and respect each LFO's velocity-independent
                // phase, free/tempo sync and per-note retrigger settings.
                const double octaveOffset = 5.0 * (filterEnvDepth[0] * modEnvelopeValues[0]
                                                  + filterEnvDepth[1] * modEnvelopeValues[1])
                    + 4.0 * (filterLfoCutoff[0] * mod + filterLfoCutoff[1] * mod2);
                const double maxCutoff = juce::jmin(18000.0, rate * 0.45);
                const double cutoffHz = juce::jlimit(20.0, maxCutoff,
                    currentCutoff * (octaveOffset != 0.0 ? std::exp2(octaveOffset) : 1.0));
                const double g = std::tan(juce::MathConstants<double>::pi * cutoffHz / rate);
                // Keep the two cascaded 24dB sections from amplifying extreme
                // resonances excessively; single-stage modes remain expressive.
                const bool twoStages = filterMode == 2 || filterMode == 4;
                const double modulatedResonance = juce::jlimit(0.0, 1.0,
                    currentResonance + 0.5 * (filterLfoResonance[0] * mod
                                               + filterLfoResonance[1] * mod2));
                const double q = 0.70710678 + modulatedResonance * (twoStages ? 1.8 : 5.0);
                const double k = 1.0 / q;
                auto& first = v.filterIntegrators[0];
                const auto response = processSvf(voiceSample, g, k, first.ic1eq, first.ic2eq);
                double filtered = voiceSample;
                if (filterMode == 1 || filterMode == 2) filtered = response.low;
                else if (filterMode == 3 || filterMode == 4) filtered = response.high;
                else if (filterMode == 5) filtered = response.band;
                else if (filterMode == 6) filtered = response.notch;
                if (twoStages) {
                    auto& second = v.filterIntegrators[1];
                    const auto secondResponse = processSvf(filtered, g, k, second.ic1eq, second.ic2eq);
                    filtered = filterMode == 2 ? secondResponse.low : secondResponse.high;
                }
                voiceSample += currentFilterMix * (filtered - voiceSample);
            }
            mix += voiceSample;
        }
        const float sample = (float)(std::tanh(mix * output));
        for (int c = 0; c < buffer.getNumChannels(); ++c) buffer.setSample(c, s, sample);
    }
    // Once per block, publish the actual LFO phases for the UI.
    // In per-note mode preview the oldest active voice's modulation.
    for (size_t i = 0; i < lfoPhases.size(); ++i) {
        double previewPhase = lfoPhases[i];
        if (noteRetrigger[i]) {
            for (const auto& v : voices) {
                if (v.note >= 0) {
                    previewPhase = v.noteLfoPhases[i];
                    break;
                }
            }
        }
        const double offsetPhase = previewPhase + lfoStartOffsets[i];
        const double fraction = (offsetPhase - twoPi * std::floor(offsetPhase / twoPi)) / twoPi;
        lfoDisplayPhases[i].store((float)fraction, std::memory_order_relaxed);
    }
}
const juce::String FMEmancipationAudioProcessor::getProgramName(int index)
{
    static const char* families[] = {
        "Electric Piano", "Bell", "Bass", "Lead", "Pad",
        "Pluck", "Organ", "Keys", "Percussion", "Texture"
    };
    if (index < 0 || index >= 100) return {};
    return juce::String(families[index / 10]) + " " + juce::String(index % 10 + 1).paddedLeft('0', 2);
}
void FMEmancipationAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= 100) return;
    currentProgram = index;
    const int family = index / 10;
    const int variation = index % 10;
    const float brightness = (float)variation / 9.0f;
    auto set = [this](const juce::String& id, float value) {
        if (auto* parameter = parameters.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    set("algorithm", (float)(1 + (family * 2 + variation) % 20));
    set("fmDepth", 0.7f + brightness * 5.5f + (family % 3) * 0.5f);
    set("amDepth", family == 1 || family == 8 ? 0.35f : 0.05f);
    set("output", 0.3f);
    set("filterMode", 0.0f);
    set("filterCutoff", 9000.0f);
    set("filterResonance", 0.15f);
    set("filterMix", 1.0f);
    set("filterEnv1", 0.0f);
    set("filterEnv2", 0.0f);
    set("filterLfo1Cutoff", 0.0f);
    set("filterLfo2Cutoff", 0.0f);
    set("filterLfo1Resonance", 0.0f);
    set("filterLfo2Resonance", 0.0f);
    set("lfoWave", 0.0f);
    set("lfoRate", 5.0f);
    set("lfoSync", 0.0f);
    set("lfoDivision", 2.0f);
    set("lfoDepth", 0.0f);
    set("lfoTarget", 0.0f);
    set("lfo2Wave", 0.0f);
    set("lfo2Rate", 1.0f);
    set("lfo2Sync", 0.0f);
    set("lfo2Division", 2.0f);
    set("lfo2Depth", 0.0f);
    set("lfo2Target", 0.0f);
    set("lfoPhaseOffset", 0.0f);
    set("lfo2PhaseOffset", 0.0f);
    set("lfoRetrigger", 0.0f);
    set("lfo2Retrigger", 0.0f);
    for (int i = 0; i < 2; ++i) {
        const auto base = "modEnv" + juce::String(i + 1);
        set(base + "Attack", 0.01f);
        set(base + "Decay", 0.35f);
        set(base + "Sustain", 0.7f);
        set(base + "Release", 0.4f);
        set(base + "Amount", 0.0f);
        set(base + "Target", 0.0f);
    }
    for (int i = 0; i < 6; ++i) {
        const auto op = juce::String(i);
        set("lfoOpPitch" + op, 0.0f);
        set("lfoOpLevel" + op, 0.0f);
        set("lfoOpFM" + op, 0.0f);
        set("lfoOpFeedback" + op, 0.0f);
        set("lfo2OpPitch" + op, 0.0f);
        set("lfo2OpLevel" + op, 0.0f);
        set("lfo2OpFM" + op, 0.0f);
        set("lfo2OpFeedback" + op, 0.0f);
    }
    set("attack", family == 4 || family == 9 ? 0.5f + brightness : 0.005f);
    set("release", family == 4 || family == 9 ? 2.0f + brightness : 0.25f);
    for (int i = 0; i < 6; ++i) {
        const auto suffix = juce::String(i);
        const float ratio = (float)(1 + ((family + i + variation / 3) % 8));
        set("ratio" + suffix, family == 3 ? (float)(i + 1) : ratio);
        set("level" + suffix, i == 0 ? 0.85f : 0.25f + 0.09f * (float)((i + variation) % 6));
        const bool sustained = family == 4 || family == 6 || family == 9;
        set("opAttack" + suffix, sustained ? 0.2f + 0.08f * i : 0.002f + 0.004f * i);
        set("opDecay" + suffix, 0.15f + 0.12f * i + 0.03f * variation);
        set("opSustain" + suffix, sustained ? 0.65f + 0.04f * (i % 3) : (family == 1 || family == 8 ? 0.0f : 0.3f));
        set("opRelease" + suffix, sustained ? 1.2f + 0.15f * i : 0.18f + 0.07f * i);
    }
}
void FMEmancipationAudioProcessor::getStateInformation(juce::MemoryBlock& data)
{
    auto state = parameters.copyState();
    state.setProperty("selectedFactoryProgram", currentProgram, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, data);
}
void FMEmancipationAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(parameters.state.getType())) {
        auto restored = juce::ValueTree::fromXml(*xml);
        currentProgram = juce::jlimit(0, 99, (int)restored.getProperty("selectedFactoryProgram", 0));
        parameters.replaceState(restored);
    }
}
juce::AudioProcessorEditor* FMEmancipationAudioProcessor::createEditor()
{
    return new FMEmancipationAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FMEmancipationAudioProcessor(); }
