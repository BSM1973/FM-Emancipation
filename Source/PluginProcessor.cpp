#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
constexpr double twoPi = juce::MathConstants<double>::twoPi;
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
    for (int i = 0; i < 6; ++i) {
        f("ratio" + juce::String(i), "OP " + juce::String(i + 1) + " ratio", 0.25f, 16, i == 2 ? 3.0f : 1.0f);
        f("level" + juce::String(i), "OP " + juce::String(i + 1) + " level", 0, 1, i < 2 ? 0.8f : 0.5f);
        f("opAttack" + juce::String(i), "OP " + juce::String(i + 1) + " attack", 0.001f, 5.0f, 0.01f);
        f("opDecay" + juce::String(i), "OP " + juce::String(i + 1) + " decay", 0.001f, 8.0f, 0.25f);
        f("opSustain" + juce::String(i), "OP " + juce::String(i + 1) + " sustain", 0.0f, 1.0f, 1.0f);
        f("opRelease" + juce::String(i), "OP " + juce::String(i + 1) + " release", 0.01f, 8.0f, 0.35f);
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
void FMEmancipationAudioProcessor::prepareToPlay(double sr, int) { rate = sr > 0 ? sr : 44100.0; voices = {}; }
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
    double ratios[6], levels[6], opAttack[6], opDecay[6], opSustain[6], opRelease[6];
    for (int i = 0; i < 6; ++i) {
        ratios[i] = param("ratio" + juce::String(i));
        levels[i] = param("level" + juce::String(i));
        opAttack[i] = param("opAttack" + juce::String(i));
        opDecay[i] = param("opDecay" + juce::String(i));
        opSustain[i] = param("opSustain" + juce::String(i));
        opRelease[i] = param("opRelease" + juce::String(i));
    }
    juce::MidiBuffer::Iterator iterator(midi);
    juce::MidiMessage message;
    int eventSample = 0;
    bool hasEvent = iterator.getNextEvent(message, eventSample);
    for (int s = 0; s < buffer.getNumSamples(); ++s) {
        while (hasEvent && eventSample <= s) {
            if (message.isNoteOn()) {
                Voice* slot = nullptr;
                for (auto& v : voices) if (v.note < 0) { slot = &v; break; }
                if (!slot) slot = &voices[0];
                *slot = {};
                slot->note = message.getNoteNumber();
                slot->velocity = message.getFloatVelocity();
            } else if (message.isNoteOff()) {
                for (auto& v : voices)
                    if (v.note == message.getNoteNumber()) v.releasing = true;
            } else if (message.isAllNotesOff() || message.isAllSoundOff()) {
                for (auto& v : voices) v = {};
            }
            hasEvent = iterator.getNextEvent(message, eventSample);
        }
        double mix = 0.0;
        for (auto& v : voices) {
            if (v.note < 0) continue;
            const double step = 1.0 / (rate * (v.releasing ? release : attack));
            v.envelope = v.releasing ? juce::jmax(0.0, v.envelope - step)
                                     : juce::jmin(1.0, v.envelope + step);
            if (v.releasing && v.envelope <= 0) { v = {}; continue; }
            const double hz = juce::MidiMessage::getMidiNoteInHertz(v.note);
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
                const double wave = std::sin(v.phase[i] + modulation[i] * depth);
                op[i] = wave * levels[i] * env;
                if (route[i] < 0) {
                    carrierSum += op[i];
                    ++carrierCount;
                } else {
                    modulation[route[i]] += op[i];
                }
                v.phase[i] += twoPi * hz * ratios[i] / rate;
                if (v.phase[i] >= twoPi) v.phase[i] = std::fmod(v.phase[i], twoPi);
            }
            const double sum = carrierSum / juce::jmax(1, carrierCount);
            const double amSource = 0.5 + 0.5 * op[5];
            mix += (sum / carrierCount) * ((1.0 - am) + am * amSource) * v.envelope * v.velocity;
        }
        const float sample = (float)(std::tanh(mix * output));
        for (int c = 0; c < buffer.getNumChannels(); ++c) buffer.setSample(c, s, sample);
    }
}
void FMEmancipationAudioProcessor::getStateInformation(juce::MemoryBlock& data)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, data);
}
void FMEmancipationAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* FMEmancipationAudioProcessor::createEditor()
{
    return new FMEmancipationAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FMEmancipationAudioProcessor(); }
