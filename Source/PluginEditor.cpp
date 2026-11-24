#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

// Separate dark-blue modulation matrix: displayed only when requested.
// All 24 controls attach directly to the processor's persistent parameters.
class FMLfoOperatorMatrix final : public juce::Component
{
public:
    explicit FMLfoOperatorMatrix(FMEmancipationAudioProcessor& processor, int source)
        : sourceNumber(source)
    {
        setSize(850, 524);
        static const char* lfo1Prefixes[] = { "lfoOpPitch", "lfoOpLevel", "lfoOpFM", "lfoOpFeedback" };
        static const char* lfo2Prefixes[] = { "lfo2OpPitch", "lfo2OpLevel", "lfo2OpFM", "lfo2OpFeedback" };
        const auto* parameterPrefixes = sourceNumber == 2 ? lfo2Prefixes : lfo1Prefixes;
        static const char* tooltips[] = {
            "Bipolaire : variation de hauteur jusqu'a +/- 2 demi-tons",
            "Bipolaire : modulation du niveau sonore de cet operateur",
            "Bipolaire : modulation de l'indice FM de cet operateur",
            "Bipolaire : modulation du feedback de cet operateur"
        };
        for (int op = 0; op < 6; ++op) {
            for (int column = 0; column < 4; ++column) {
                auto& slider = sliders[(size_t)(op * 4 + column)];
                slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
                slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff78c9fa));
                slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff37576b));
                slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffc6e9ff));
                slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffc6e9ff));
                slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
                slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff465869));
                slider.setTooltip(tooltips[column]);
                addAndMakeVisible(slider);
                attachments.push_back(
                    std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                        processor.parameters, juce::String(parameterPrefixes[column]) + juce::String(op), slider));
            }
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff192b3a));
        g.setColour(juce::Colour(0xff78c9fa));
        g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
        g.drawText("MATRICE DE MODULATION - LFO " + juce::String(sourceNumber),
                   22, 14, 780, 32, juce::Justification::left);
        g.setColour(juce::Colour(0xffc4e5ff));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Destination : MATRICE (6 OP)  |  Reglages bipolaires : -100 % a +100 %",
                   22, 48, 805, 22, juce::Justification::left);
        static const char* headings[] = { "HAUTEUR", "NIVEAU", "FM", "FEEDBACK" };
        g.setColour(juce::Colour(0xff77bfe6));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        for (int column = 0; column < 4; ++column)
            g.drawText(headings[column], 145 + column * 170, 80, 148, 22, juce::Justification::centred);
        for (int op = 0; op < 6; ++op) {
            const int y = 107 + op * 68;
            g.setColour(juce::Colour(op % 2 == 0 ? 0xff233c4e : 0xff203448));
            g.fillRoundedRectangle(14.0f, (float)y, 822.0f, 63.0f, 5.0f);
            g.setColour(juce::Colour(0xffc4e5ff));
            g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
            g.drawText("OP " + juce::String(op + 1), 28, y + 18, 103, 26, juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        for (int op = 0; op < 6; ++op)
            for (int column = 0; column < 4; ++column)
                sliders[(size_t)(op * 4 + column)].setBounds(163 + column * 170,
                                                              109 + op * 68, 110, 59);
    }

private:
    int sourceNumber = 1;
    std::array<juce::Slider, 24> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
};


class FMLfoPhaseModePanel final : public juce::Component
{
public:
    explicit FMLfoPhaseModePanel(FMEmancipationAudioProcessor& processor)
    {
        setSize(615, 275);
        for (int i = 0; i < 2; ++i) {
            const auto prefix = i == 0 ? juce::String("lfo") : juce::String("lfo2");
            auto& phase = phaseKnobs[(size_t)i];
            phase.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            phase.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
            phase.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff70d6f5));
            phase.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff37576b));
            phase.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe3f6ff));
            phase.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe3f6ff));
            phase.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
            phase.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff465869));
            phase.setTooltip("Position initiale : 0 = 0 degre ; 0.25 = 90 degres ; 1 = 360 degres");
            addAndMakeVisible(phase);
            phaseAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, prefix + "PhaseOffset", phase));

            auto& trigger = triggerSelectors[(size_t)i];
            trigger.addItem("Continu / global", 1);
            trigger.addItem("A chaque note", 2);
            trigger.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff174568));
            trigger.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
            trigger.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
            trigger.setTooltip("Chaque voix MIDI redemarre son LFO sans modifier les autres notes tenues");
            addAndMakeVisible(trigger);
            triggerAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.parameters, prefix + "Retrigger", trigger));
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff192b3a));
        g.setColour(juce::Colour(0xff78c9fa));
        g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
        g.drawText("PHASE ET DECLENCHEMENT DES LFO", 22, 14, 555, 34, juce::Justification::left);
        g.setColour(juce::Colour(0xffb5c6d6));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText("PHASE INITIALE", 222, 53, 136, 20, juce::Justification::centred);
        g.drawText("DECLENCHEMENT", 394, 53, 190, 20, juce::Justification::centred);
        for (int i = 0; i < 2; ++i) {
            const int y = 78 + i * 82;
            g.setColour(juce::Colour(i == 0 ? 0xff233c4e : 0xff203448));
            g.fillRoundedRectangle(16.0f, (float)y, 582.0f, 78.0f, 5.0f);
            g.setColour(i == 0 ? juce::Colour(0xff70d6f5) : juce::Colour(0xff9cb3ff));
            g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
            g.drawText("LFO " + juce::String(i + 1), 35, y + 24, 126, 26, juce::Justification::centredLeft);
        }
        g.setColour(juce::Colour(0xffb5c6d6));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Mode libre : choisir SYNC > Libre dans la section de modulation.",
                   22, 247, 568, 19, juce::Justification::centredLeft);
    }

    void resized() override
    {
        for (int i = 0; i < 2; ++i) {
            phaseKnobs[(size_t)i].setBounds(233, 80 + i * 82, 108, 74);
            triggerSelectors[(size_t)i].setBounds(402, 103 + i * 82, 176, 30);
        }
    }

private:
    std::array<juce::Slider, 2> phaseKnobs;
    std::array<juce::ComboBox, 2> triggerSelectors;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> phaseAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> triggerAttachments;
};


class FMDualModulationEnvelopePanel final : public juce::Component
{
public:
    explicit FMDualModulationEnvelopePanel(FMEmancipationAudioProcessor& processor)
    {
        setSize(1004, 428);
        static constexpr const char* suffixes[] = { "Attack", "Decay", "Sustain", "Release", "Amount" };
        for (int e = 0; e < 2; ++e) {
            const auto base = "modEnv" + juce::String(e + 1);
            for (int j = 0; j < 5; ++j) {
                auto& slider = controls[(size_t)e][(size_t)j];
                slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 18);
                slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff78c9fa));
                slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff34566c));
                slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe3f6ff));
                slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffc4e5ff));
                slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
                slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff465869));
                slider.onValueChange = [this] { repaint(); };
                addAndMakeVisible(slider);
                sliderAttachments.push_back(
                    std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                        processor.parameters, base + suffixes[j], slider));
            }
            auto& target = destinations[(size_t)e];
            target.addItemList({ "Desactivee", "Hauteur", "Profondeur FM",
                                 "Profondeur AM", "Niveau", "Feedback" }, 1);
            target.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff174568));
            target.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
            target.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
            target.setTooltip("Destination de l'enveloppe : modulation polyphonique par note");
            addAndMakeVisible(target);
            targetAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.parameters, base + "Target", target));
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff192b3a));
        g.setColour(juce::Colour(0xff78c9fa));
        g.setFont(juce::FontOptions(23.0f, juce::Font::bold));
        g.drawText("ENVELOPPES DE MODULATION", 24, 14, 820, 34, juce::Justification::left);
        g.setColour(juce::Colour(0xff9ebfd5));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Deux ADSR independantes des six operateurs  /  declenchement polyphonique MIDI",
                   24, 53, 940, 20, juce::Justification::left);
        static constexpr const char* labels[] = { "ATTAQUE", "DECAY", "SUSTAIN", "RELEASE", "INTENSITE" };
        for (int e = 0; e < 2; ++e) {
            const int top = 85 + e * 157;
            g.setColour(juce::Colour(e == 0 ? 0xff223c4e : 0xff213648));
            g.fillRoundedRectangle(14.0f, (float)top, 976.0f, 151.0f, 6.0f);
            g.setColour(juce::Colour(0xff46657c));
            g.drawRoundedRectangle(14.0f, (float)top, 976.0f, 151.0f, 6.0f, 1.0f);
            g.setColour(e == 0 ? juce::Colour(0xff70d6f5) : juce::Colour(0xff9cb3ff));
            g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
            g.drawText("ENV " + juce::String(e + 1), 31, top + 48, 100, 30, juce::Justification::left);
            g.setColour(juce::Colour(0xffb5c6d6));
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            for (int j = 0; j < 5; ++j)
                g.drawText(labels[j], 143 + j * 108, top + 9, 92, 20, juce::Justification::centred);
            g.drawText("DESTINATION", 740, top + 24, 215, 22, juce::Justification::centred);

            // Miniature of the actual ADSR shape from the current settings.
            const float attack = (float)controls[(size_t)e][0].getValue();
            const float decay = (float)controls[(size_t)e][1].getValue();
            const float sustain = (float)controls[(size_t)e][2].getValue();
            const float release = (float)controls[(size_t)e][3].getValue();
            const float total = attack + decay + 0.35f + release;
            const float startX = 151.0f, y = (float)top + 140.0f;
            const float graphWidth = 515.0f, graphHeight = 24.0f;
            const float atX = startX + graphWidth * attack / total;
            const float decX = atX + graphWidth * decay / total;
            const float susX = decX + graphWidth * 0.35f / total;
            juce::Path curve;
            curve.startNewSubPath(startX, y);
            curve.lineTo(atX, y - graphHeight);
            curve.lineTo(decX, y - graphHeight * sustain);
            curve.lineTo(susX, y - graphHeight * sustain);
            curve.lineTo(startX + graphWidth, y);
            g.setColour(juce::Colour(0xff405d71));
            g.drawLine(startX, y, startX + graphWidth, y, 1.0f);
            g.setColour(e == 0 ? juce::Colour(0xff70d6f5) : juce::Colour(0xff9cb3ff));
            g.strokePath(curve, juce::PathStrokeType(1.6f));
        }
        g.setColour(juce::Colour(0xffa8c5d8));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Intensite : -100 % a +100 %  |  Les nouvelles enveloppes sont desactivees par defaut.",
                   22, 402, 950, 20, juce::Justification::left);
    }

    void resized() override
    {
        for (int e = 0; e < 2; ++e) {
            const int top = 85 + e * 157;
            for (int j = 0; j < 5; ++j)
                controls[(size_t)e][(size_t)j].setBounds(147 + j * 108, top + 31, 84, 80);
            destinations[(size_t)e].setBounds(742, top + 60, 215, 34);
        }
    }

private:
    std::array<std::array<juce::Slider, 5>, 2> controls;
    std::array<juce::ComboBox, 2> destinations;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> targetAttachments;
};


class FMMultimodeFilterPanel final : public juce::Component
{
public:
    explicit FMMultimodeFilterPanel(FMEmancipationAudioProcessor& processor)
    {
        setSize(926, 615);
        filterMode.addItemList({
            "DÉSACTIVÉ", "PASSE-BAS 12 dB", "PASSE-BAS 24 dB",
            "PASSE-HAUT 12 dB", "PASSE-HAUT 24 dB",
            "PASSE-BANDE 12 dB", "COUPE-BANDE 12 dB"
        }, 1);
        filterMode.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff174568));
        filterMode.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
        filterMode.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
        filterMode.onChange = [this] { repaint(); };
        addAndMakeVisible(filterMode);
        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "filterMode", filterMode);

        static constexpr const char* paramIds[] = {
            "filterCutoff", "filterResonance", "filterMix", "filterEnv1", "filterEnv2"
        };
        for (int i = 0; i < 5; ++i) {
            auto& slider = controls[(size_t)i];
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 22);
            slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff70d6f5));
            slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff36576e));
            slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffd4f2ff));
            slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffc4e5ff));
            slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
            slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff465869));
            slider.onValueChange = [this] { repaint(); };
            addAndMakeVisible(slider);
            sliderAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, paramIds[i], slider));
        }
        controls[0].setTextValueSuffix(" Hz");
        controls[0].setTooltip("Fréquence de coupure du filtre, de 25 Hz à 18 kHz");
        controls[1].setTooltip("Résonance autour de la fréquence de coupure");
        controls[2].setTooltip("Dosage de signal filtré, de 0 à 100 %");
        controls[3].setTooltip("Modulation bipolaire de la coupure par l'enveloppe ENV 1");
        controls[4].setTooltip("Modulation bipolaire de la coupure par l'enveloppe ENV 2");
        static constexpr const char* lfoRoutingIds[] = {
            "filterLfo1Cutoff", "filterLfo2Cutoff",
            "filterLfo1Resonance", "filterLfo2Resonance"
        };
        for (int i = 0; i < 4; ++i) {
            auto& slider = lfoRoutes[(size_t)i];
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
            slider.setColour(juce::Slider::rotarySliderFillColourId,
                i % 2 == 0 ? juce::Colour(0xff70d6f5) : juce::Colour(0xff9cb3ff));
            slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff36576e));
            slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffd4f2ff));
            slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffc4e5ff));
            slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
            slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff465869));
            slider.setTooltip(i < 2
                ? "Modulation bipolaire de la coupure : +/- 4 octaves au maximum"
                : "Modulation bipolaire de la resonance, limitee a la plage du filtre");
            addAndMakeVisible(slider);
            sliderAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, lfoRoutingIds[i], slider));
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff192b3a));
        g.setColour(juce::Colour(0xff78c9fa));
        g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
        g.drawText("FILTRE MULTIMODE", 26, 18, 670, 35, juce::Justification::left);
        g.setColour(juce::Colour(0xffb5c6d6));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Filtre par voix • 12/24 dB • ENV 1 et ENV 2 • mélange progressif",
                   26, 55, 860, 22, juce::Justification::left);

        g.setColour(juce::Colour(0xff243d50));
        g.fillRoundedRectangle(18.0f, 88.0f, 890.0f, 127.0f, 6.0f);
        g.setColour(juce::Colour(0xff4b708a));
        g.drawRoundedRectangle(18.0f, 88.0f, 890.0f, 127.0f, 6.0f, 1.0f);

        g.setColour(juce::Colour(0xffc8e7f9));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText("TYPE", 30, 102, 182, 20, juce::Justification::centred);
        static constexpr const char* labels[] = {
            "COUPURE", "RÉSONANCE", "MIX", "ENV 1 → CUT", "ENV 2 → CUT"
        };
        for (int i = 0; i < 5; ++i)
            g.drawText(labels[i], 233 + 132 * i, 100, 116, 20, juce::Justification::centred);

        const auto bounds = juce::Rectangle<float>(43.0f, 247.0f, 840.0f, 163.0f);
        g.setColour(juce::Colour(0xff112838));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(juce::Colour(0xff4b708a));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
        const float graphLeft = 75.0f, graphRight = 870.0f;
        const float graphTop = 278.0f, graphBottom = 394.0f;
        for (int db = -36; db <= 12; db += 12) {
            const float y = graphBottom - (db + 48.0f) / 60.0f * (graphBottom - graphTop);
            g.setColour(juce::Colour(0xff294759));
            g.drawHorizontalLine(juce::roundToInt(y), graphLeft, graphRight);
            g.setColour(juce::Colour(0xff8da8b9));
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(juce::String(db) + " dB", 44, juce::roundToInt(y) - 8, 42, 14,
                       juce::Justification::left);
        }
        auto graphX = [&](double hz) {
            return graphLeft + (float)(std::log(hz / 25.0) / std::log(18000.0 / 25.0))
                               * (graphRight - graphLeft);
        };
        for (const auto hz : { 100.0, 1000.0, 10000.0 }) {
            const float x = graphX(hz);
            g.setColour(juce::Colour(0xff294759));
            g.drawVerticalLine(juce::roundToInt(x), graphTop, graphBottom);
            g.setColour(juce::Colour(0xff8da8b9));
            g.drawText(hz == 100.0 ? "100 Hz" : hz == 1000.0 ? "1 kHz" : "10 kHz",
                       juce::roundToInt(x) - 32, 399, 66, 15, juce::Justification::centred);
        }

        const int mode = juce::jlimit(0, 6, filterMode.getSelectedId() - 1);
        const double cutoff = juce::jlimit(25.0, 18000.0, controls[0].getValue());
        const double resonance = juce::jlimit(0.0, 1.0, controls[1].getValue());
        const bool cascaded = mode == 2 || mode == 4;
        const double q = 0.70710678 + resonance * (cascaded ? 1.8 : 5.0);
        const float cursorX = graphX(cutoff);
        juce::Path response;
        for (int point = 0; point <= 220; ++point) {
            const double freq = 25.0 * std::pow(18000.0 / 25.0, (double)point / 220.0);
            const double ratio = freq / cutoff;
            const double square = ratio * ratio;
            const double denominator = std::sqrt((1.0 - square) * (1.0 - square)
                                                 + square / (q * q));
            double magnitude = 1.0;
            if (mode == 1 || mode == 2) magnitude = 1.0 / denominator;
            else if (mode == 3 || mode == 4) magnitude = square / denominator;
            else if (mode == 5) magnitude = (ratio / q) / denominator;
            else if (mode == 6) magnitude = std::abs(1.0 - square) / denominator;
            if (cascaded) magnitude *= magnitude;
            const double db = 20.0 * std::log10(juce::jmax(1.0e-6, magnitude));
            const float y = graphBottom - (float)((juce::jlimit(-48.0, 12.0, db) + 48.0) / 60.0)
                                                    * (graphBottom - graphTop);
            const float x = graphLeft + (graphRight - graphLeft) * (float)point / 220.0f;
            if (point == 0) response.startNewSubPath(x, y);
            else response.lineTo(x, y);
        }
        g.setColour(juce::Colour(0xff70d6f5));
        g.strokePath(response, juce::PathStrokeType(2.2f));
        g.setColour(juce::Colour(0xffd7efff).withAlpha(0.75f));
        g.drawLine(cursorX, graphTop, cursorX, graphBottom, 1.2f);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("RÉPONSE DU FILTRE  /  APERÇU INDICATIF", 48, 224, 440, 18,
                   juce::Justification::left);
        g.setColour(juce::Colour(0xffa7c7da));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("Les enveloppes et LFO modulent le filtre en temps réel. La courbe indique sa réponse de base.",
                   34, 421, 865, 18, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xff243d50));
        g.fillRoundedRectangle(18.0f, 456.0f, 890.0f, 144.0f, 6.0f);
        g.setColour(juce::Colour(0xff4b708a));
        g.drawRoundedRectangle(18.0f, 456.0f, 890.0f, 144.0f, 6.0f, 1.0f);
        g.setColour(juce::Colour(0xff78c9fa));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("MODULATIONS LFO — FILTRE", 30, 466, 435, 20, juce::Justification::left);
        static constexpr const char* routingNames[] = {
            "LFO 1 → COUPURE", "LFO 2 → COUPURE", "LFO 1 → RÉSONANCE", "LFO 2 → RÉSONANCE"
        };
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        for (int i = 0; i < 4; ++i) {
            g.setColour(i % 2 == 0 ? juce::Colour(0xff70d6f5) : juce::Colour(0xff9cb3ff));
            g.drawText(routingNames[i], 53 + 218 * i, 486, 180, 18,
                       juce::Justification::centred);
        }
        g.setColour(juce::Colour(0xffa7c7da));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("Routages indépendants des destinations LFO. Régler l'intensité du LFO au-dessus de zéro.",
                   34, 594, 865, 16, juce::Justification::centredLeft);
    }

    void resized() override
    {
        filterMode.setBounds(31, 146, 180, 34);
        for (int i = 0; i < 5; ++i)
            controls[(size_t)i].setBounds(239 + i * 132, 124, 104, 84);
        for (int i = 0; i < 4; ++i)
            lfoRoutes[(size_t)i].setBounds(90 + 218 * i, 506, 110, 82);
    }

private:
    juce::ComboBox filterMode;
    std::array<juce::Slider, 5> controls;
    std::array<juce::Slider, 4> lfoRoutes;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
};



class FMStereoEffectsModulation final : public juce::Component
{
public:
    explicit FMStereoEffectsModulation(FMEmancipationAudioProcessor& processor)
    {
        setSize(860, 475);
        static constexpr const char* sources[] { "Lfo1", "Lfo2", "Env1", "Env2" };
        static constexpr const char* targets[] { "Chorus", "Delay", "Reverb" };
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 3; ++col) {
                auto& slider = knobs[(size_t)(row * 3 + col)];
                slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
                slider.setColour(juce::Slider::rotarySliderFillColourId,
                    row % 2 ? juce::Colour(0xff9cb3ff) : juce::Colour(0xff70d6f5));
                slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff36576e));
                slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffc4e5ff));
                slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
                slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff465869));
                addAndMakeVisible(slider);
                attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, juce::String("fxMod") + sources[row] + targets[col], slider));
            }
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff192b3a));
        g.setColour(juce::Colour(0xff78c9fa));
        g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
        g.drawText("MATRICE DE MODULATION DES EFFETS", 22, 12, 800, 32, juce::Justification::left);
        g.setFont(juce::FontOptions(12.0f));
        g.setColour(juce::Colour(0xffafc9dc));
        g.drawText("Routages bipolaires : -100 % à +100 %. Zéro = désactivé.", 22, 49, 790, 22,
                   juce::Justification::left);
        static constexpr const char* cols[] { "PROFONDEUR CHORUS", "FEEDBACK DELAY", "MIX RÉVERBÉRATION" };
        for (int col = 0; col < 3; ++col) {
            g.setColour(juce::Colour(0xffc4e5ff));
            g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            g.drawText(cols[col], 190 + col * 213, 78, 195, 20, juce::Justification::centred);
        }
        static constexpr const char* rows[] { "LFO 1", "LFO 2", "ENV 1", "ENV 2" };
        for (int row = 0; row < 4; ++row) {
            const int top = 104 + row * 84;
            g.setColour(juce::Colour(row % 2 ? 0xff203749 : 0xff253d50));
            g.fillRoundedRectangle(15.0f, (float)top, 827.0f, 80.0f, 5.0f);
            g.setColour(row % 2 ? juce::Colour(0xff9cb3ff) : juce::Colour(0xff70d6f5));
            g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
            g.drawText(rows[row], 31, top + 24, 125, 26, juce::Justification::left);
        }
        g.setColour(juce::Colour(0xffafc9dc));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("Les LFO suivent le tempo ou les notes MIDI ; les enveloppes suivent les voix.",
                   22, 450, 816, 18, juce::Justification::left);
    }
    void resized() override
    {
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 3; ++col)
                knobs[(size_t)(row * 3 + col)].setBounds(230 + col * 213,
                                                        108 + row * 84, 108, 73);
    }
private:
    std::array<juce::Slider, 12> knobs;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
};
class FMStereoEffectsPanel final : public juce::Component
{
public:
    explicit FMStereoEffectsPanel(FMEmancipationAudioProcessor& processor)
    {
        setSize(1000, 570);
        matrixButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff174568));
        matrixButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
        matrixButton.setTooltip("Matrice LFO/ENV vers Chorus, Delay et Réverbération");
        matrixButton.onClick = [this, &processor] {
            juce::CallOutBox::launchAsynchronously(
                std::make_unique<FMStereoEffectsModulation>(processor),
                getLocalArea(&matrixButton, matrixButton.getLocalBounds()), this);
        };
        addAndMakeVisible(matrixButton);
        static constexpr const char* sliderIds[] = {
            "fxChorusRate", "fxChorusDepth", "fxChorusMix",
            "fxDelayTime", "fxDelayFeedback", "fxDelayMix",
            "fxReverbSize", "fxReverbDamping", "fxReverbWidth", "fxReverbMix"
        };
        for (int i = 0; i < 10; ++i) {
            auto& slider = knobs[(size_t)i];
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
            slider.setColour(juce::Slider::rotarySliderFillColourId,
                i < 3 ? juce::Colour(0xff72d6f5)
                      : i < 6 ? juce::Colour(0xff8caaf9) : juce::Colour(0xff78c9fa));
            slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff37576b));
            slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffd4f2ff));
            slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffc4e5ff));
            slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
            slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff465869));
            addAndMakeVisible(slider);
            sliderAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, sliderIds[i], slider));
        }
        knobs[0].setTextValueSuffix(" Hz");
        knobs[3].setTextValueSuffix(" s");
        static constexpr const char* switchIds[] = {
            "fxChorusEnabled", "fxDelayEnabled", "fxReverbEnabled",
            "fxDelaySync", "fxDelayDivision", "fxDelayPingPong"
        };
        for (int i = 0; i < 6; ++i) {
            auto& combo = switches[(size_t)i];
            if (i == 4)
                combo.addItemList({ "1 mesure", "1/2", "1/4", "1/8",
                                    "1/8 pointée", "1/8 triolet" }, 1);
            else if (i == 3)
                combo.addItemList({ "Libre", "Tempo" }, 1);
            else if (i == 5)
                combo.addItemList({ "Stéréo", "Ping-pong" }, 1);
            else
                combo.addItemList({ "Désactivé", "Activé" }, 1);
            combo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff174568));
            combo.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
            combo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
            addAndMakeVisible(combo);
            comboAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.parameters, switchIds[i], combo));
        }
        knobs[3].setTooltip("Temps libre du delay (secondes) ; en mode Tempo, utiliser DIVISION");
        knobs[4].setTooltip("Quantité de répétitions du delay, avec limitation à 85 %");
        knobs[9].setTooltip("Dosage de la réverbération stéréo");
        switches[5].setTooltip("Mode stéréo classique ou alternance gauche/droite");
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff192b3a));
        g.setColour(juce::Colour(0xff78c9fa));
        g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
        g.drawText("EFFETS STÉRÉO", 24, 12, 650, 34, juce::Justification::centredLeft);
        g.setFont(juce::FontOptions(12.0f));
        g.setColour(juce::Colour(0xffafc9dc));
        g.drawText("CHORUS  →  DELAY STÉRÉO  →  RÉVERBÉRATION   •   Chaque effet possède son propre interrupteur.",
                   24, 48, 940, 22, juce::Justification::centredLeft);

        static constexpr const char* names[] = { "CHORUS", "DELAY STÉRÉO", "RÉVERBÉRATION" };
        static constexpr int top[] = { 79, 227, 399 };
        static constexpr int height[] = { 137, 161, 146 };
        const auto blue = juce::Colour(0xff78c9fa);
        for (int section = 0; section < 3; ++section) {
            const int y = top[section];
            g.setColour(juce::Colour(section == 1 ? 0xff243a50 : 0xff223849));
            g.fillRoundedRectangle(16.0f, (float)y, 968.0f, (float)height[section], 7.0f);
            g.setColour(juce::Colour(0xff45647c));
            g.drawRoundedRectangle(16.0f, (float)y, 968.0f, (float)height[section], 7.0f, 1.0f);
            g.setColour(section == 1 ? juce::Colour(0xffa5b8ff) : blue);
            g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
            g.drawText(names[section], 34, y + 8, 500, 26, juce::Justification::centredLeft);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.setColour(juce::Colour(0xffbfdced));
            g.drawText("ÉTAT", 37, y + 44, 150, 16, juce::Justification::centred);
        }

        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xffbfdced));
        static constexpr const char* chorusLabels[] = { "VITESSE", "PROFONDEUR", "MÉLANGE" };
        for (int i = 0; i < 3; ++i)
            g.drawText(chorusLabels[i], 222 + i * 165, 101, 120, 18, juce::Justification::centred);

        static constexpr const char* delayLabels[] = { "TEMPS", "FEEDBACK", "MÉLANGE" };
        for (int i = 0; i < 3; ++i)
            g.drawText(delayLabels[i], 205 + i * 142, 251, 110, 18, juce::Justification::centred);
        g.drawText("SYNC", 645, 254, 106, 16, juce::Justification::centred);
        g.drawText("DIVISION", 755, 254, 108, 16, juce::Justification::centred);
        g.drawText("MODE", 870, 254, 108, 16, juce::Justification::centred);

        static constexpr const char* reverbLabels[] = { "TAILLE", "AMORTISSEMENT", "LARGEUR", "MÉLANGE" };
        for (int i = 0; i < 4; ++i)
            g.drawText(reverbLabels[i], 220 + i * 180, 421, 140, 18, juce::Justification::centred);

        g.setColour(juce::Colour(0xff8baec5));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("Queues naturelles de delay/réverbération après les notes ; modulation via MATRICE MOD.",
                   25, 547, 950, 18, juce::Justification::centredLeft);
    }

    void resized() override
    {
        matrixButton.setBounds(780, 17, 185, 30);
        switches[0].setBounds(38, 145, 150, 32); // CHORUS
        switches[1].setBounds(38, 302, 150, 32); // DELAY
        switches[2].setBounds(38, 468, 150, 32); // REVERB
        for (int i = 0; i < 3; ++i)
            knobs[(size_t)i].setBounds(230 + i * 165, 122, 105, 86);
        for (int i = 0; i < 3; ++i)
            knobs[(size_t)(3 + i)].setBounds(210 + i * 142, 275, 102, 87);
        knobs[6].setBounds(226, 441, 110, 86);
        knobs[7].setBounds(406, 441, 110, 86);
        knobs[8].setBounds(586, 441, 110, 86);
        knobs[9].setBounds(766, 441, 110, 86);
        switches[3].setBounds(645, 296, 108, 32); // SYNC
        switches[4].setBounds(758, 296, 110, 32); // DIVISION
        switches[5].setBounds(874, 296, 105, 32); // PING PONG
    }

private:
    juce::TextButton matrixButton { "MATRICE MOD" };
    std::array<juce::Slider, 10> knobs;
    std::array<juce::ComboBox, 6> switches;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
};


class FMArpeggiatorPanel final : public juce::Component, private juce::Timer
{
public:
    explicit FMArpeggiatorPanel(FMEmancipationAudioProcessor& processor)
        : synth(processor)
    {
        setSize(780, 372);
        auto combo = [&](juce::ComboBox& box, const juce::String& id,
                         const juce::StringArray& values) {
            box.addItemList(values, 1);
            box.setComponentID(id);
            box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff174568));
            box.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
            box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
            addAndMakeVisible(box);
            combos.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                synth.parameters, id, box));
        };
        combo(enabled, "arpEnabled", { "DÉSACTIVÉ", "ACTIVÉ" });
        combo(mode, "arpMode", { "MONTANT", "DESCENDANT", "ALLER / RETOUR", "ALÉATOIRE", "ACCORDS" });
        combo(division, "arpDivision", { "1 MESURE", "1/2", "1/4", "1/8", "1/16", "1/32", "1/8 TRIOLET" });
        combo(octaves, "arpOctaves", { "1 OCTAVE", "2 OCTAVES", "3 OCTAVES", "4 OCTAVES" });
        for (int i = 0; i < 2; ++i) {
            auto& knob = i == 0 ? swing : gate;
            knob.setComponentID(i == 0 ? "arpSwing" : "arpGate");
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 20);
            knob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff70d6f5));
            knob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff37576b));
            knob.setColour(juce::Slider::thumbColourId, juce::Colour(0xffd4f2ff));
            knob.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffc4e5ff));
            knob.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
            knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff465869));
            addAndMakeVisible(knob);
            sliders.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                synth.parameters, i == 0 ? "arpSwing" : "arpGate", knob));
        }
        swing.setTooltip("Décale un pas sur deux pour créer un rythme swing");
        gate.setTooltip("Longueur des notes entre 5 % et 100 % de chaque pas");
        startTimerHz(15);
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff192b3a));
        g.setColour(juce::Colour(0xff78c9fa));
        g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
        g.drawText("ARPÉGIATEUR", 22, 14, 620, 38, juce::Justification::left);
        g.setFont(juce::FontOptions(12.0f));
        g.setColour(juce::Colour(0xffafc9dc));
        g.drawText("Synchronisé au tempo du DAW  •  Notes MIDI maintenues  •  Jusqu'à 4 octaves",
                   22, 54, 720, 22, juce::Justification::left);
        g.setColour(juce::Colour(0xff223c4e));
        g.fillRoundedRectangle(16.0f, 90.0f, 748.0f, 211.0f, 7.0f);
        g.setColour(juce::Colour(0xff4b708a));
        g.drawRoundedRectangle(16.0f, 90.0f, 748.0f, 211.0f, 7.0f, 1.0f);
        g.setColour(juce::Colour(0xffc4e5ff));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText("MARCHE", 34, 107, 132, 20, juce::Justification::centred);
        g.drawText("DIRECTION", 192, 107, 210, 20, juce::Justification::centred);
        g.drawText("DIVISION", 431, 107, 138, 20, juce::Justification::centred);
        g.drawText("OCTAVES", 600, 107, 140, 20, juce::Justification::centred);
        g.drawText("SWING", 243, 178, 112, 20, juce::Justification::centred);
        g.drawText("GATE", 450, 178, 112, 20, juce::Justification::centred);
        g.setColour(juce::Colour(0xff70d6f5));
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        const int note = synth.getArpNoteForDisplay();
        g.drawText(note >= 0 ? "NOTE : " + juce::MidiMessage::getMidiNoteName(note, true, true, 3)
                             : "NOTE : —", 34, 224, 166, 32, juce::Justification::left);
        g.setColour(juce::Colour(0xff9ebbd0));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Sans note enfoncée, l'arpégiateur reste silencieux. Désactiver restaure le MIDI normal.",
                   24, 317, 720, 22, juce::Justification::left);
        g.drawText("Les réglages sont sauvegardés avec les presets et automatisables dans le DAW.",
                   24, 342, 720, 20, juce::Justification::left);
    }
    void resized() override
    {
        enabled.setBounds(33, 134, 134, 32);
        mode.setBounds(194, 134, 210, 32);
        division.setBounds(433, 134, 138, 32);
        octaves.setBounds(600, 134, 140, 32);
        swing.setBounds(246, 201, 106, 88);
        gate.setBounds(452, 201, 106, 88);
    }
private:
    void timerCallback() override { repaint(24, 222, 180, 37); }
    FMEmancipationAudioProcessor& synth;
    juce::ComboBox enabled, mode, division, octaves;
    juce::Slider swing, gate;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliders;
};


namespace {
constexpr int designW=1504, designH=1006;
const juce::Colour panel(0xff273039), edge(0xff45535f), ink(0xffd4e5ef), muted(0xff9aafbf), cyan(0xff83cfff), well(0xff1b252e);
juce::String utf(const char* s){ return juce::String::fromUTF8(s); }
void text(juce::Graphics& g, const juce::String& s, juce::Rectangle<int> b, float size=14, juce::Colour colour=ink, bool bold=false, juce::Justification j=juce::Justification::centredLeft) {
    g.setColour(colour); g.setFont(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain)); g.drawFittedText(s,b,j,1);
}
void frame(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour fill=panel, juce::Colour line=edge, float radius=4) {
    g.setColour(fill); g.fillRoundedRectangle(b,radius); g.setColour(line); g.drawRoundedRectangle(b,radius,1);
}
class HardwareLook final : public juce::LookAndFeel_V4 {
public:
    HardwareLook(){
        setColour(juce::PopupMenu::backgroundColourId,well); setColour(juce::PopupMenu::textColourId,ink);
        setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff315775));
        setColour(juce::PopupMenu::highlightedTextColourId,juce::Colours::white);
        setColour(juce::ComboBox::backgroundColourId,well); setColour(juce::ComboBox::textColourId,ink);
        setColour(juce::ComboBox::outlineColourId,edge); setColour(juce::Label::textColourId,ink);
        setColour(juce::TextEditor::backgroundColourId,well); setColour(juce::TextEditor::textColourId,ink);
        setColour(juce::TextEditor::outlineColourId,edge);
    }
    juce::Font getTextButtonFont(juce::TextButton&,int h) override {return juce::Font(juce::FontOptions(juce::jlimit(11.0f,15.0f,h*0.34f),juce::Font::bold));}
    juce::Font getComboBoxFont(juce::ComboBox&) override {return juce::Font(juce::FontOptions(14.0f));}
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& c,bool hover,bool down) override {
        auto r=b.getLocalBounds().toFloat().reduced(0.7f);
        auto top=c.isTransparent()?juce::Colour(0xff394650):c;
        if(hover)top=top.brighter(0.14f); if(down)top=top.darker(0.25f);
        g.setGradientFill(juce::ColourGradient(top,r.getX(),r.getY(),top.darker(0.45f),r.getX(),r.getBottom(),false));
        g.fillRoundedRectangle(r,3);g.setColour(b.getToggleState()?cyan:edge);g.drawRoundedRectangle(r,3,1);
        if(b.getToggleState()){g.setColour(cyan);g.fillRect(r.withTop(r.getBottom()-3));}
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) override {
        const float d=juce::jmin((float)w,(float)h)-6.0f;
        if(d<=4)return;
        auto r=juce::Rectangle<float>(d,d).withCentre({x+w*0.5f,y+h*0.5f});
        const auto c=r.getCentre(); const float rad=d*0.44f;
        g.setColour(juce::Colour(0xff111a22));g.fillEllipse(r);
        juce::Path arc;arc.addCentredArc(c.x,c.y,rad,rad,0,start,end,true);
        g.setColour(juce::Colour(0xff10171d));g.strokePath(arc,juce::PathStrokeType(4));
        juce::Path active;active.addCentredArc(c.x,c.y,rad,rad,0,start,start+pos*(end-start),true);
        g.setColour(cyan);g.strokePath(active,juce::PathStrokeType(2.7f));
        auto cap=r.reduced(d*0.15f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3b4955),cap.getX(),cap.getY(),juce::Colour(0xff2a343e),cap.getRight(),cap.getBottom(),false));
        g.fillEllipse(cap);g.setColour(edge);g.drawEllipse(cap,1);
        const float a=start+pos*(end-start);auto p=c.getPointOnCircumference(d*0.29f,a);
        auto q=c.getPointOnCircumference(d*0.18f,a);
        g.setColour(ink);g.drawLine(juce::Line<float>(p,q),2.6f);
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle style,juce::Slider&) override{
        bool vert=style==juce::Slider::LinearVertical;
        if(vert){
            auto track=juce::Rectangle<float>((float)x+w*0.5f-9,(float)y+2,18,(float)h-4);
            frame(g,track,juce::Colour(0xff112e40),juce::Colour(0xff49768d),1.5f);
            auto thumb=juce::Rectangle<float>((float)x+w*0.5f-15,pos-3.5f,30,7);
            frame(g,thumb,juce::Colour(0xffa0cbe2),ink,1);
        }else{
            auto track=juce::Rectangle<float>((float)x+5,(float)y+h*0.5f-2,(float)w-10,4);
            frame(g,track,juce::Colour(0xff111a21),edge,1.5f);
            auto thumb=juce::Rectangle<float>(pos-6,(float)y+h*0.5f-7,12,14);
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd2e5ef),thumb.getX(),thumb.getY(),juce::Colour(0xff839bab),thumb.getX(),thumb.getBottom(),false));
            g.fillRoundedRectangle(thumb,1);g.setColour(ink);g.drawRoundedRectangle(thumb,1,0.8f);
        }
    }
};
class Hit final:public juce::Button{
public:
    Hit():juce::Button("control"){}
    void paintButton(juce::Graphics& g,bool over,bool down) override {
        if(over||down){g.setColour(cyan.withAlpha(down?0.17f:0.08f));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1),3);}
    }
};
const char* const algorithmNames[]={"Deep Ladder","Branch Ladder","Twin Stacks","Split Ladder","Radiant Fan","Crossfire","Three Pairs","Open Trio","Triple Branch","Open Quartet","Hybrid Tower","Twin Tines","Hybrid Fan","Hybrid Pairs","Spectral Ladder","AM Pairs","AM Cascade","AM Tower","AM Quartet","Six Suns"};
const char* const categoryFR[]={"Tous les sons","Pianos électriques","Claviers et cloches","Basses","Leads","Nappes","Plucks et maillets","Cuivres et vents","Orgues et clavs","Effets sonores","FM-AM expérimental","Favoris","Mes presets"};
const char* const categoryEN[]={"All sounds","Electric Pianos","Acoustic and Bell Keys","Basses","Leads","Pads","Plucks and Mallets","Brass and Winds","Organs and Clavs","Sound Effects","Experimental FM-AM","Favourites","User presets"};
}
class FMEmancipationAudioProcessorEditor::Surface final:public juce::Component,private juce::Timer {
public:
    explicit Surface(FMEmancipationAudioProcessor& p):synth(p){
        setSize(designW,designH);setLookAndFeel(&look);addMouseListener(this,true);
        for(int i=0;i<5;++i){auto& b=tabs[(size_t)i]; configure(b);b.onClick=[this,i]{page=i;update();};b.setBounds(35+i*192,279,181,47);}
        configure(prev);configure(next);configure(star);configure(menu);configure(lang);
        prev.setBounds(483,214,34,30);next.setBounds(528,214,34,30);star.setBounds(438,214,34,30);menu.setBounds(573,214,35,30);
        lang.setBounds(1452,10,39,23);prev.setButtonText(utf("‹"));next.setButtonText(utf("›"));menu.setButtonText("...");
        prev.onClick=[this]{step(-1);};next.onClick=[this]{step(1);};star.onClick=[this]{toggleFavourite(synth.getCurrentProgram());};menu.onClick=[this]{advancedMenu();};
        lang.onClick=[this]{synth.english.store(!synth.english.load());localise();update();};
        for(int i=0;i<6;++i){
            const auto s=juce::String(i);
            setupSlider(level[(size_t)i],"level"+s,juce::Slider::LinearVertical,false);
            level[(size_t)i].setBounds(1030+i*69,164,53,71);
            for(int k=0;k<5;++k){
                const char* ids[]={"ratio","fine","feedback","velocity","keyLevel"};
                auto& sl=ops[(size_t)(i*5+k)];
                setupSlider(sl,juce::String(ids[k])+s,juce::Slider::LinearHorizontal,false);
                const int ys[]={433,484,614,665,716};sl.setBounds(53+i*238,ys[k],203,24);
            }
            auto& w=waves[(size_t)i];setupCombo(w,"wave"+s,{"Sine","Triangle","Saw","Square"});w.setBounds(54+i*238,542,202,34);
        }
        const char* masterIds[]={"fmDepth","amDepth","modAmount","globalFeedback","globalVelocity","keyTrack","lfoRate","vibrato","tremolo","lfoToMod","transpose","tune","output","pan","width"};
        const int mx[]={102,210,317,424,543,625,707,789,871,953,1068,1154,1240,1326,1412};
        for(int i=0;i<15;++i){auto& s=master[(size_t)i];setupSlider(s,masterIds[i],juce::Slider::RotaryHorizontalVerticalDrag,true);s.setBounds(mx[i]-34,840,68,86);}
        master[12].textFromValueFunction=[](double v){return juce::String(juce::Decibels::gainToDecibels(v,-90.0),1);};
        master[12].valueFromTextFunction=[](const juce::String& v){return juce::Decibels::decibelsToGain(v.getDoubleValue(),-90.0);};
        for(int i=0;i<10;++i){auto& b=algoHits[(size_t)i];addAndMakeVisible(b);b.setBounds(54+(i%5)*175,i<5?360:589,165,87);b.onClick=[this,i]{setParam("algorithm",1+algoPage*10+i);repaint();};}
        for(auto* b:{&algoPrev,&algoNext,&envPrev,&envNext,&catPrev,&catNext,&presetPrev,&presetNext})configure(*b);
        algoPrev.setBounds(55,721,34,30);algoNext.setBounds(91,721,34,30);envPrev.setBounds(1345,348,35,24);envNext.setBounds(1384,348,35,24);
        catPrev.setBounds(53,731,36,26);catNext.setBounds(94,731,36,26);presetPrev.setBounds(1078,738,36,25);presetNext.setBounds(1118,738,36,25);
        for(auto* b:{&algoPrev,&envPrev,&catPrev,&presetPrev})b->setButtonText(utf("‹"));
        for(auto* b:{&algoNext,&envNext,&catNext,&presetNext})b->setButtonText(utf("›"));
        algoPrev.onClick=[this]{algoPage=0;repaint();};algoNext.onClick=[this]{algoPage=1;repaint();};
        envPrev.onClick=[this]{envPage=0;layoutEnvelopes();repaint();};envNext.onClick=[this]{envPage=1;layoutEnvelopes();repaint();};
        catPrev.onClick=[this]{catPage=juce::jmax(0,catPage-1);update();};catNext.onClick=[this]{catPage=juce::jmin(2,catPage+1);update();};
        presetPrev.onClick=[this]{presetPage=juce::jmax(0,presetPage-1);update();};presetNext.onClick=[this]{if((presetPage+1)*12<(int)entries.size())++presetPage;update();};
        const char* globalIds[]={"attack","ampDecay","ampSustain","release","pitchAttack","pitchDecay","pitchSustain","pitchRelease","pitchAmount"};
        for(int i=0;i<9;++i){auto& s=globalEnv[(size_t)i];setupSlider(s,globalIds[i],juce::Slider::LinearHorizontal,false);s.setBounds(i<4?58:284, i<4?491+i*37:491+(i-4)*37,206,24);}
        const char* envIds[]={"opAttack","opDecay","opSustain","opRelease"};
        for(int i=0;i<6;++i)for(int k=0;k<4;++k)setupSlider(opEnv[(size_t)(i*4+k)],juce::String(envIds[k])+juce::String(i),juce::Slider::LinearHorizontal,false);
        const char* fxIds[]={"fxChorusMix","fxChorusRate","fxChorusDepth","fxDelayMix","fxDelayTime","fxDelayFeedback","fxReverbMix","fxReverbDecay","fxReverbSize","fxReverbDamping"};
        const int fxX[]={166,382,166,645,860,645,1123,1340,1123,1340};
        for(int i=0;i<10;++i){auto& s=fx[(size_t)i];setupSlider(s,fxIds[i],juce::Slider::RotaryHorizontalVerticalDrag,true);s.setBounds(fxX[i]-45,(i==2||i==5||i>=8)?649:503,90,98);}
        const char* onIds[]={"fxChorusEnabled","fxDelayEnabled","fxReverbEnabled"};
        for(int i=0;i<3;++i){auto& b=fxOn[(size_t)i];configure(b);b.setComponentID(onIds[i]);b.setBounds(423+i*478,368,57,36);b.onClick=[this,i]{const char* ids[]={"fxChorusEnabled","fxDelayEnabled","fxReverbEnabled"};setParam(ids[i],value(ids[i])>=0.5f?0:1);repaint();};}
        for(int i=0;i<6;++i){auto& h=categoryHits[(size_t)i];addAndMakeVisible(h);h.setBounds(51,357+i*61,249,59);h.onClick=[this,i]{const int c=catPage*6+i;if(c<13){category=c;presetPage=0;refresh();update();}};}
        for(int i=0;i<12;++i){auto& h=presetHits[(size_t)i];auto& fav=presetStars[(size_t)i];addAndMakeVisible(h);addAndMakeVisible(fav);const int x=337+(i%2)*423,y=419+(i/2)*53;h.setBounds(x,y,377,50);fav.setBounds(x+377,y,36,50);
            h.onClick=[this,i]{const int row=presetPage*12+i;if(row<(int)entries.size())load(entries[(size_t)row]);};
            fav.onClick=[this,i]{const int row=presetPage*12+i;if(row<(int)entries.size()&&entries[(size_t)row].factory>=0)toggleFavourite(entries[(size_t)row].factory);};}
        setupEdit(search);search.setBounds(337,359,846,46);search.onTextChange=[this]{presetPage=0;refresh();update();};
        setupEdit(name);name.setBounds(1223,403,226,45);
        configure(save);configure(rename);configure(remove);save.setBounds(1223,459,226,45);rename.setBounds(1223,515,226,45);remove.setBounds(1223,571,226,45);
        save.onClick=[this]{saveUser(false);};rename.onClick=[this]{saveUser(true);};remove.onClick=[this]{deleteUser();};
        localise();refresh();update();startTimerHz(20);
    }
    ~Surface() override{stopTimer();removeMouseListener(this);setLookAndFeel(nullptr);}
    void paint(juce::Graphics& g) override{
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff242a31),0,0,juce::Colour(0xff191f25),1504,1006,false));g.fillAll();
        g.setColour(edge);g.drawRoundedRectangle(1,2,1502,1002,8,1);
        text(g,"FM",{35,28,66,50},38,juce::Colours::whitesmoke,true);
        text(g,"Emancipation",{95,28,355,50},38,cyan,true);
        text(g,tr("SIX OPÉRATEURS · EXPRESSION INFINIE","SIX OPERATORS · INFINITE EXPRESSION"),{36,89,620,19},14,muted);
        text(g,tr("ARCHITECTURE","ARCHITECTURE"),{948,40,138,18},13,muted,true);
        text(g,tr("ROUTAGE","ROUTING"),{1090,40,159,18},13,muted,true);
        g.setColour(juce::Colour(0xff87d2ba));g.fillEllipse(1265,44,10,10);
        text(g,tr("POLYPHONIQUE","POLYPHONIC"),{1282,40,167,18},13,muted,true);
        text(g,"FM × AM",{948,66,138,24},18,ink,true);
        text(g,tr("20 ALGORITHMES","20 ALGORITHMS"),{1090,66,170,24},17,ink,true);
        text(g,tr("INSTRUMENT NUMÉRIQUE","DIGITAL INSTRUMENT"),{1265,66,218,24},17,ink,true);
        drawLCD(g);
        text(g,tr("MOTEUR FM / AM","FM / AM ENGINE"),{1300,288,168,24},13,muted,false,juce::Justification::centredRight);
        frame(g,{36,342,1432,424});
        if(page==0)drawOperators(g);
        if(page==1)drawAlgorithms(g);
        if(page==2)drawEnvelopes(g);
        if(page==3)drawEffects(g);
        if(page==4)drawPresets(g);
        drawMaster(g);
        text(g,tr("FM EMANCIPATION / SYNTHÉTISEUR FM–AM À SIX OPÉRATEURS","FM EMANCIPATION / SIX-OPERATOR FM–AM SYNTHESIZER"),{36,947,840,22},11,muted,true);
        text(g,tr("MIDI HÔTE · AUTOMATION · MÉMORISATION","HOST MIDI · AUTOMATION · STATE RECALL"),{900,947,568,22},11,muted,true,juce::Justification::centredRight);
    }
    void mouseDown(const juce::MouseEvent& e) override{
        if(!e.mods.isPopupMenu())return;
        auto* c=e.eventComponent;while(c&&c!=this&&c->getComponentID().isEmpty())c=c->getParentComponent();
        if(!c||c==this)return;const auto id=c->getComponentID();if(!synth.parameters.getParameter(id))return;
        juce::PopupMenu m;m.addItem(1,tr("Apprendre MIDI CC","MIDI Learn"));const int cc=synth.midiBinding(id);
        m.addItem(2,tr("Supprimer l’affectation","Remove mapping"),cc>=0);
        if(cc>=0)m.addItem(3,"CC "+juce::String(cc%128)+" / CH "+juce::String(cc/128+1),false);
        m.addItem(4,tr("Annuler l’apprentissage","Cancel learn"));
        auto safe=juce::Component::SafePointer<Surface>(this);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(c),[safe,id](int r){if(safe==nullptr)return;if(r==1)safe->synth.armMidiLearn(id);if(r==2)safe->synth.clearMidiLearn(id);if(r==4)safe->synth.armMidiLearn({});safe->repaint();});
    }
private:
    using SA=juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    struct Entry{int factory=-1;juce::File file;juce::String label;};
    FMEmancipationAudioProcessor& synth;HardwareLook look;
    int page=0,algoPage=0,envPage=0,category=0,catPage=0,presetPage=0,lastProgram=-1;
    juce::File selectedUser;juce::String userTitle;
    std::array<juce::TextButton,5> tabs;
    juce::TextButton prev,next,star,menu,lang,algoPrev,algoNext,envPrev,envNext,catPrev,catNext,presetPrev,presetNext,save,rename,remove;
    std::array<juce::TextButton,3> fxOn;
    std::array<juce::Slider,6> level;
    std::array<juce::Slider,30> ops;
    std::array<juce::ComboBox,6> waves;
    std::array<juce::Slider,15> master;
    std::array<juce::Slider,9> globalEnv;
    std::array<juce::Slider,24> opEnv;
    std::array<juce::Slider,10> fx;
    std::array<Hit,10> algoHits;
    std::array<Hit,6> categoryHits;
    std::array<Hit,12> presetHits,presetStars;
    juce::TextEditor search,name;
    std::vector<Entry> entries;
    std::vector<std::unique_ptr<SA>> sliders;
    std::vector<std::unique_ptr<CA>> combos;
    juce::String tr(const char* fr,const char* en)const{return utf(synth.english.load()?en:fr);}
    juce::String cat(int i)const{return utf((synth.english.load()?categoryEN:categoryFR)[juce::jlimit(0,12,i)]);}
    float value(const juce::String& id)const{if(auto* p=synth.parameters.getRawParameterValue(id))return p->load();return 0;}
    void setParam(const juce::String& id,float v){if(auto* p=synth.parameters.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));p->endChangeGesture();}}
    int algorithm()const{return juce::jlimit(0,19,(int)std::lround(value("algorithm"))-1);}
    void configure(juce::TextButton& b){b.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff35414c));b.setColour(juce::TextButton::textColourOffId,ink);b.setColour(juce::TextButton::textColourOnId,ink);addAndMakeVisible(b);}
    void setupEdit(juce::TextEditor& e){e.setFont(juce::Font(juce::FontOptions(15.0f)));e.setColour(juce::TextEditor::backgroundColourId,well);e.setColour(juce::TextEditor::textColourId,ink);e.setColour(juce::TextEditor::outlineColourId,edge);addAndMakeVisible(e);}
    void setupSlider(juce::Slider& s,const juce::String& id,juce::Slider::SliderStyle style,bool numeric){
        s.setComponentID(id);s.setName(id);s.setSliderStyle(style);s.setPopupDisplayEnabled(false,false,nullptr);
        s.setTextBoxStyle(numeric?juce::Slider::TextBoxBelow:juce::Slider::NoTextBox,false,64,27);
        s.setColour(juce::Slider::textBoxTextColourId,ink);s.setColour(juce::Slider::textBoxBackgroundColourId,well);s.setColour(juce::Slider::textBoxOutlineColourId,edge);
        addAndMakeVisible(s);sliders.push_back(std::make_unique<SA>(synth.parameters,id,s));
        s.setNumDecimalPlacesToDisplay(2);
        if(auto* p=synth.parameters.getParameter(id))s.setDoubleClickReturnValue(true,p->convertFrom0to1(p->getDefaultValue()));
    }
    void setupCombo(juce::ComboBox& box,const juce::String& id,const juce::StringArray& names){box.setComponentID(id);box.addItemList(names,1);addAndMakeVisible(box);combos.push_back(std::make_unique<CA>(synth.parameters,id,box));}
    void localise(){
        const char* fr[]={"01  OPÉRATEURS","02  ALGORITHME","03  ENVELOPPES","04  EFFETS","05  PRESETS"};
        const char* en[]={"01  OPERATORS","02  ALGORITHM","03  ENVELOPES","04  EFFECTS","05  PRESETS"};
        for(int i=0;i<5;++i)tabs[(size_t)i].setButtonText(tr(fr[i],en[i]));
        lang.setButtonText(synth.english.load()?"EN":"FR");lang.setTooltip(tr("Changer de langue","Change language"));
        star.setTooltip(tr("Favori","Favourite"));menu.setTooltip(tr("Fonctions avancées","Advanced functions"));
        for(auto& w:waves){for(int i=0;i<4;++i){const char* a[]={"Sinus","Triangle","Scie","Carrée"};const char* b[]={"Sine","Triangle","Saw","Square"};w.changeItemText(i+1,tr(a[i],b[i]));}}
        search.setTextToShowWhenEmpty(tr("Rechercher des programmes…","Search programs…"),muted);
        name.setTextToShowWhenEmpty(tr("Nom de votre son…","Name your sound…"),muted);
        save.setButtonText(tr("ENREGISTRER SOUS","SAVE CURRENT AS"));rename.setButtonText(tr("RENOMMER LE PRESET","RENAME USER PRESET"));remove.setButtonText(tr("SUPPRIMER LE PRESET","DELETE USER PRESET"));
    }
    void layoutEnvelopes(){
        for(int i=0;i<6;++i)for(int k=0;k<4;++k){auto& s=opEnv[(size_t)(i*4+k)];const int x=531+(i%3)*318+(k%2)*150;
            s.setBounds(x,448+(k/2)*46,135,24);s.setVisible(page==2&&i/3==envPage);}
    }
    void update(){
        for(int i=0;i<5;++i){tabs[(size_t)i].setToggleState(i==page,juce::dontSendNotification);tabs[(size_t)i].setColour(juce::TextButton::buttonColourId,i==page?juce::Colour(0xff49667a):juce::Colour(0xff35414b));}
        for(auto& s:ops)s.setVisible(page==0);for(auto& s:waves)s.setVisible(page==0);
        for(auto& b:algoHits)b.setVisible(page==1);algoPrev.setVisible(page==1);algoNext.setVisible(page==1);
        for(auto& s:globalEnv)s.setVisible(page==2);envPrev.setVisible(page==2);envNext.setVisible(page==2);layoutEnvelopes();
        for(auto& s:fx)s.setVisible(page==3);for(auto& b:fxOn)b.setVisible(page==3);
        for(int i=0;i<6;++i){auto& h=categoryHits[(size_t)i];h.setVisible(page==4&&catPage*6+i<13);if(catPage*6+i<13)h.setName(cat(catPage*6+i));}
        for(int i=0;i<12;++i){const int k=presetPage*12+i;const bool exists=k<(int)entries.size();presetHits[(size_t)i].setVisible(page==4&&exists);presetStars[(size_t)i].setVisible(page==4&&exists&&entries[(size_t)k].factory>=0);if(exists)presetHits[(size_t)i].setName(entries[(size_t)k].label);}
        for(auto* c:{static_cast<juce::Component*>(&search),static_cast<juce::Component*>(&name),static_cast<juce::Component*>(&save),static_cast<juce::Component*>(&rename),static_cast<juce::Component*>(&remove),static_cast<juce::Component*>(&catPrev),static_cast<juce::Component*>(&catNext),static_cast<juce::Component*>(&presetPrev),static_cast<juce::Component*>(&presetNext)})c->setVisible(page==4);
        rename.setEnabled(selectedUser.existsAsFile());remove.setEnabled(selectedUser.existsAsFile());catPrev.setEnabled(catPage>0);catNext.setEnabled(catPage<2);presetPrev.setEnabled(presetPage>0);presetNext.setEnabled((presetPage+1)*12<(int)entries.size());
        repaint();
    }
    void timerCallback()override{
        const int program=synth.getCurrentProgram();if(program!=lastProgram){if(lastProgram>=0){selectedUser=juce::File();userTitle.clear();}lastProgram=program;}
        star.setButtonText(synth.favourites[(size_t)juce::jlimit(0,99,program)].load()?utf("★"):utf("☆"));
        const char* ids[]={"fxChorusEnabled","fxDelayEnabled","fxReverbEnabled"};
        for(int i=0;i<3;++i){const bool on=value(ids[i])>=0.5f;auto& b=fxOn[(size_t)i];b.setButtonText(on?utf("● ON"):"OFF");b.setColour(juce::TextButton::textColourOffId,on?juce::Colour(0xff9ddcc6):muted);}
        repaint();
    }
    void step(int dir){selectedUser=juce::File();userTitle.clear();synth.setCurrentProgram((synth.getCurrentProgram()+100+dir)%100);lastProgram=synth.getCurrentProgram();refresh();update();}
    void toggleFavourite(int i){if(i<0||i>=100)return;auto& f=synth.favourites[(size_t)i];f.store(!f.load());refresh();update();}
    juce::File folder()const{return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("FM Emancipation");}
    void refresh(){
        entries.clear();auto q=search.getText().trim();
        if(category!=12)for(int i=0;i<100;++i){if(category==11&&!synth.favourites[(size_t)i].load())continue;if(category>=1&&category<=10&&i/10!=category-1)continue;
            auto n=synth.getProgramName(i);if(q.isEmpty()||n.containsIgnoreCase(q)||cat(i/10+1).containsIgnoreCase(q))entries.push_back({i,{},n});}
        if(category==0||category==12){auto files=folder().findChildFiles(juce::File::findFiles,true,"*.fme");for(auto& f:files){auto n=f.getFileNameWithoutExtension();if(q.isEmpty()||n.containsIgnoreCase(q))entries.push_back({-1,f,n});}}
        presetPage=juce::jlimit(0,juce::jmax(0,((int)entries.size()-1)/12),presetPage);
    }
    void load(Entry e){
        if(e.factory>=0){selectedUser=juce::File();userTitle.clear();synth.setCurrentProgram(e.factory);}
        else{juce::MemoryBlock b;if(!e.file.loadFileAsData(b)||b.isEmpty()){notice(tr("Preset illisible","Cannot read preset"));return;}
            const bool language=synth.english.load();std::array<bool,100> fav{};for(int i=0;i<100;++i)fav[(size_t)i]=synth.favourites[(size_t)i].load();
            synth.setStateInformation(b.getData(),(int)b.getSize());synth.english.store(language);for(int i=0;i<100;++i)synth.favourites[(size_t)i].store(fav[(size_t)i]);
            selectedUser=e.file;userTitle=e.label;name.setText(e.label,juce::dontSendNotification);}
        lastProgram=synth.getCurrentProgram();algoPage=algorithm()/10;refresh();update();
    }
    void notice(const juce::String& message){juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,"FM Emancipation",message,"OK");}
    void saveUser(bool renaming){
        const auto n=juce::File::createLegalFileName(name.getText().trim());if(n.isEmpty()){notice(tr("Saisissez un nom.","Enter a name."));return;}
        if(!folder().createDirectory()){notice(tr("Dossier inaccessible.","Cannot create folder."));return;}
        auto target=(renaming?selectedUser.getParentDirectory():folder()).getChildFile(n+".fme");
        if(target.exists()){notice(tr("Ce nom existe déjà.","This name already exists."));return;}
        bool ok=false;if(renaming){if(selectedUser.existsAsFile())ok=selectedUser.moveFileTo(target);}
        else{juce::MemoryBlock b;synth.getStateInformation(b);ok=target.replaceWithData(b.getData(),b.getSize());}
        if(!ok){notice(tr("Impossible d’enregistrer.","Could not save."));return;}selectedUser=target;userTitle=n;refresh();update();
    }
    void deleteUser(){
        if(!selectedUser.existsAsFile())return;auto f=selectedUser;auto safe=juce::Component::SafePointer<Surface>(this);
        juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,"FM Emancipation",tr("Supprimer ce preset ?","Delete this preset?")+"\n"+f.getFileName(),tr("Supprimer","Delete"),tr("Annuler","Cancel"),this,
            juce::ModalCallbackFunction::create([safe,f](int r){if(safe!=nullptr&&r!=0){if(f.moveToTrash()){safe->selectedUser=juce::File();safe->userTitle.clear();safe->name.clear();safe->refresh();safe->update();}else safe->notice(safe->tr("Suppression impossible.","Could not delete."));}}));
    }
    void advancedMenu(){
        juce::PopupMenu m;m.addItem(1,tr("Arpégiateur MIDI","MIDI arpeggiator"));m.addItem(2,tr("Filtre multimode","Multimode filter"));m.addItem(3,tr("Enveloppes de modulation","Modulation envelopes"));
        m.addItem(4,tr("Matrice LFO 1","LFO 1 matrix"));m.addItem(5,tr("Matrice LFO 2","LFO 2 matrix"));m.addItem(6,tr("Phase / mode LFO","LFO phase / mode"));m.addItem(7,tr("Effets avancés","Advanced effects"));m.addItem(8,tr("Réglages des deux LFO","Dual LFO settings"));m.addSeparator();m.addItem(9,tr("English","Français"));
        auto safe=juce::Component::SafePointer<Surface>(this);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menu),[safe](int r){if(safe==nullptr||r==0)return;if(r==9){safe->synth.english.store(!safe->synth.english.load());safe->localise();safe->update();return;}
            std::unique_ptr<juce::Component> c;auto& p=safe->synth;
            if(r==1)c=std::make_unique<FMArpeggiatorPanel>(p);if(r==2)c=std::make_unique<FMMultimodeFilterPanel>(p);if(r==3)c=std::make_unique<FMDualModulationEnvelopePanel>(p);
            if(r==4||r==5)c=std::make_unique<FMLfoOperatorMatrix>(p,r-3);if(r==6)c=std::make_unique<FMLfoPhaseModePanel>(p);if(r==7)c=std::make_unique<FMStereoEffectsPanel>(p);if(r==8)c=safe->makeLfoPanel();
            if(c){auto* parent=safe->getParentComponent();if(parent)juce::CallOutBox::launchAsynchronously(std::move(c),parent->getLocalArea(safe.getComponent(),safe->menu.getBounds()),parent);}
        });
    }
    class LfoPanel final:public juce::Component{
    public:
        explicit LfoPanel(FMEmancipationAudioProcessor& p){
            setSize(680,300);
            const char* suffix[]={"Wave","Sync","Division","Target"};
            const juce::StringArray options[]={{"Sinus","Triangle","Scie",utf("Carrée")},{"Libre","Tempo"},{"1/1","1/2","1/4","1/8","1/16","1/8.","1/8T"},{utf("Désactivé"),"Vibrato",utf("Trémolo"),"FM","AM","Matrice"}};
            for(int n=0;n<2;++n){const juce::String prefix=n==0?"lfo":"lfo2";for(int k=0;k<4;++k){auto& b=boxes[(size_t)(n*4+k)];b.addItemList(options[k],1);b.setBounds(115+k*137,55+n*145,125,30);addAndMakeVisible(b);ca.push_back(std::make_unique<CA>(p.parameters,prefix+suffix[k],b));}
                for(int k=0;k<2;++k){auto& s=knobs[(size_t)(n*2+k)];s.setSliderStyle(juce::Slider::LinearHorizontal);s.setTextBoxStyle(juce::Slider::TextBoxRight,false,60,22);s.setBounds(115+k*260,105+n*145,245,30);addAndMakeVisible(s);sa.push_back(std::make_unique<SA>(p.parameters,prefix+(k==0?"Rate":"Depth"),s));}}
        }
        void paint(juce::Graphics& g)override{g.fillAll(panel);for(int n=0;n<2;++n){text(g,"LFO "+juce::String(n+1),{15,15+n*145,90,30},20,cyan,true);for(int k=0;k<4;++k){const char* a[]={"Wave","Sync","Division","Target"};text(g,a[k],{115+k*137,30+n*145,125,20});}text(g,"Rate",{115,83+n*145,150,20});text(g,"Depth",{375,83+n*145,150,20});}}
    private:std::array<juce::ComboBox,8> boxes;std::array<juce::Slider,4> knobs;std::vector<std::unique_ptr<CA>> ca;std::vector<std::unique_ptr<SA>> sa;
    };
    std::unique_ptr<juce::Component> makeLfoPanel(){return std::make_unique<LfoPanel>(synth);}
    void diagram(juce::Graphics& g,juce::Rectangle<float> b,int alg,bool large){
        const auto& route=FMEmancipationAudioProcessor::routes[alg];
        int depth[6]{},count[6]{},order[6]{},maxDepth=0;
        for(int i=0;i<6;++i){depth[i]=route[i]<0?0:depth[route[i]]+1;order[i]=count[depth[i]]++;maxDepth=juce::jmax(maxDepth,depth[i]);}
        juce::Point<float> pos[6];const float stepY=b.getHeight()/(maxDepth+2.0f);
        const float node=juce::jmin(large?32.0f:8.0f,stepY*0.66f);
        for(int i=0;i<6;++i){pos[i]={b.getX()+b.getWidth()*(order[i]+1.0f)/(count[depth[i]]+1.0f),b.getBottom()-stepY*(depth[i]+1.1f)};}
        for(int i=0;i<6;++i){int dst=route[i];if(dst<0)continue;int type=FMEmancipationAudioProcessor::routeType(alg,i);g.setColour(type==2?juce::Colour(0xffe8bc80):type==3?juce::Colour(0xffc49be9):cyan);
            auto a=pos[i].translated(0,node*0.5f),z=pos[dst].translated(0,-node*0.5f);g.drawLine(juce::Line<float>(a,z),large?2.8f:0.8f);
            if(large){auto d=(a-z);float len=d.getDistanceFromOrigin();if(len>1){d=d/len;auto q=z+d*9.0f;juce::Point<float> n(-d.y,d.x);g.drawLine(juce::Line<float>(z,q+n*4.0f),1.7f);g.drawLine(juce::Line<float>(z,q-n*4.0f),1.7f);}}}
        for(int i=0;i<6;++i)if(route[i]<0){g.setColour(cyan.withAlpha(0.65f));g.drawLine(pos[i].x,pos[i].y+node*0.5f,pos[i].x,b.getBottom()-stepY*0.35f,large?1.8f:0.6f);g.drawLine(pos[i].x,b.getBottom()-stepY*0.35f,b.getCentreX(),b.getBottom()-stepY*0.35f,large?1.8f:0.6f);}
        for(int i=0;i<6;++i){auto r=juce::Rectangle<float>(node,node).withCentre(pos[i]);frame(g,r,route[i]<0?juce::Colour(0xff427c9d):juce::Colour(0xff193447),cyan.withAlpha(0.75f),large?5.0f:1.0f);text(g,juce::String(i+1),r.toNearestInt(),large?18.0f:6.0f,ink,true,juce::Justification::centred);}
        if(large)text(g,"OUT",{(int)b.getCentreX()-26,(int)(b.getBottom()-stepY*0.35f)-8,52,17},10,ink,true,juce::Justification::centred);
    }
    void drawLCD(juce::Graphics& g){
        auto lcd=juce::Rectangle<float>(36,117,1430,145);frame(g,lcd,juce::Colour(0xff0f1f2a),juce::Colour(0xff33586f),5);
        auto inner=lcd.reduced(4);g.setGradientFill(juce::ColourGradient(juce::Colour(0xff174b70),inner.getX(),inner.getY(),juce::Colour(0xff103247),inner.getRight(),inner.getBottom(),false));g.fillRoundedRectangle(inner,2);
        g.setColour(juce::Colour(0xff4d93bc).withAlpha(0.25f));g.drawVerticalLine(631,120,259);g.drawVerticalLine(994,120,259);
        text(g,tr("MÉMOIRE PROGRAMME","PROGRAM MEMORY"),{61,135,390,25},13,juce::Colour(0xff9bc8e6),true);
        text(g,tr("ÉDITION","EDIT"),{530,135,78,25},13,juce::Colour(0xff9bc8e6),true,juce::Justification::centredRight);
        auto n=userTitle.isEmpty()?synth.getProgramName(synth.getCurrentProgram()):userTitle;
        text(g,n,{61,166,550,38},28,juce::Colour(0xffbce8fb),true);
        text(g,synth.midiLearnTarget()>=0?tr("MIDI Learn : bougez un contrôleur","MIDI Learn: move a controller"):userTitle.isEmpty()?cat(synth.getCurrentProgram()/10+1):tr("Programme utilisateur","User program"),{61,215,365,26},14,juce::Colour(0xff9bc8e6));
        const int a=algorithm();text(g,tr("ALGORITHME","ALGORITHM"),{648,141,210,23},15,ink,true);text(g,juce::String(a+1).paddedLeft('0',2),{648,165,160,36},29,ink,true);text(g,algorithmNames[a],{648,201,210,21},13,muted,true);
        text(g,tr("ROUTAGE FM / AM","FM / AM ROUTING"),{648,222,210,18},12,muted,true);diagram(g,{855,147,120,90},a,false);
        text(g,tr("NIVEAUX DES OPÉRATEURS","OPERATOR OUTPUT LEVELS"),{1013,128,422,18},13,juce::Colour(0xff9bc8e6),true);
        for(int i=0;i<6;++i){text(g,juce::String(i+1),{1031+i*69,143,52,18},12,ink,true,juce::Justification::centred);text(g,juce::String(juce::roundToInt(level[(size_t)i].getValue()*100)),{1031+i*69,239,52,18},12,ink,true,juce::Justification::centred);}
    }
    void drawOperators(juce::Graphics& g){
        const char* fr[]={"Rapport de fréquence","Accord fin","Forme d’onde","Feedback local","Réponse vélocité","Suivi clavier"};
        const char* en[]={"Frequency ratio","Fine tune","Waveform","Local feedback","Velocity response","Key level tracking"};
        const int y[]={408,459,510,589,640,691};const int slIndex[]={0,1,-1,2,3,4};const int a=algorithm();
        for(int i=0;i<6;++i){const int x=36+i*238; if(i) {g.setColour(edge);g.drawVerticalLine(x,343,766);}
            text(g,"OP "+juce::String(i+1),{x+17,362,120,31},23,ink,true);
            int t=FMEmancipationAudioProcessor::routeType(a,i);auto badge=juce::Rectangle<float>((float)x+158,364,68,26);frame(g,badge,well,t==0?juce::Colour(0xff557f9a):edge,2);
            text(g,t==0?tr("PORTEUR","CARRIER"):t==1?"FM MOD":t==2?"AM MOD":"FM+AM",badge.toNearestInt(),10,t==0?cyan:muted,true,juce::Justification::centred);
            for(int k=0;k<6;++k){text(g,tr(fr[k],en[k]),{x+17,y[k],150,24},13,muted);
                if(slIndex[k]>=0){auto v=ops[(size_t)(i*5+slIndex[k])].getValue();auto s=juce::String(v,k==0?2:k==1?1:3);if(k==0)s+=" ×";if(k==1)s+=" ct";text(g,s,{x+159,y[k],62,24},12,ink,true,juce::Justification::centredRight);}}
        }
    }
    void drawAlgorithms(juce::Graphics& g){
        g.setColour(edge);g.drawVerticalLine(937,343,766);const int a=algorithm();
        for(int i=0;i<10;++i){int idx=algoPage*10+i;auto r=algoHits[(size_t)i].getBounds().toFloat();frame(g,r,idx==a?juce::Colour(0xff234256):juce::Colour(0xff202a33),idx==a?cyan:edge,3);
            text(g,juce::String(idx+1).paddedLeft('0',2),{(int)r.getX()+8,(int)r.getY()+8,36,17},12,muted,true);
            diagram(g,{r.getX()+38,r.getY()+8,89,46},idx,false);text(g,algorithmNames[idx],{(int)r.getX()+5,(int)r.getY()+56,155,22},12,ink,false,juce::Justification::centred);}
        text(g,algoPage==0?"01 – 10":"11 – 20",{135,721,180,30},13,muted);
        text(g,juce::String(a+1).paddedLeft('0',2)+" / "+algorithmNames[a],{962,360,474,36},23,ink,true);
        int carriers=0;for(int i=0;i<6;++i)if(FMEmancipationAudioProcessor::routes[a][i]<0)++carriers;
        text(g,juce::String(carriers)+tr(" porteurs · "," carriers · ")+juce::String(6-carriers)+tr(" routes"," modulation routes"),{962,401,470,20},14,muted);
        text(g,tr("Les porteurs alimentent la sortie stéréo.","Highlighted operators feed the stereo output."),{962,422,470,20},13,muted);
        diagram(g,{1000,451,411,245},a,true);
        const juce::Colour colours[]={cyan,juce::Colour(0xffe8bc80),juce::Colour(0xffc49be9)};const char* labels[]={"FM","AM","FM+AM"};
        for(int i=0;i<3;++i){g.setColour(colours[i]);g.fillRect(962+i*61,708,16,3);text(g,labels[i],{982+i*61,698,52,22},12,muted,true);}
        text(g,tr("Feedback local par opérateur ; contrôle global commun.","Each operator has local feedback; global Feedback scales all six."),{962,731,485,26},13,muted);
    }
    void envelopeGraph(juce::Graphics& g,juce::Rectangle<float> b,double a,double d,double s,double r){
        g.setColour(juce::Colour(0xff34434e));for(int i=0;i<3;++i)g.drawHorizontalLine((int)(b.getY()+i*b.getHeight()/2),b.getX(),b.getRight());
        float x1=b.getX()+b.getWidth()*(0.04f+0.21f*(float)(a/(a+0.2))),x2=x1+b.getWidth()*(0.06f+0.22f*(float)(d/(d+0.4))),x3=b.getRight()-b.getWidth()*(0.04f+0.21f*(float)(r/(r+0.4)));
        float top=b.getY()+3,bottom=b.getBottom()-2,sy=bottom-(bottom-top)*(float)s;
        juce::Path p;p.startNewSubPath(b.getX(),bottom);p.lineTo(x1,top);p.lineTo(x2,sy);p.lineTo(juce::jmax(x2,x3),sy);p.lineTo(b.getRight(),bottom);g.setColour(cyan);g.strokePath(p,juce::PathStrokeType(2));g.fillEllipse(x1-2,top-2,4,4);g.fillEllipse(x2-2,sy-2,4,4);
    }
    void drawEnvelopes(juce::Graphics& g){
        for(int x:{513,831,1149}){g.setColour(edge);g.drawVerticalLine(x,343,766);}
        const char* fr[]={"Attaque","Déclin","Maintien","Relâchement","Hauteur (st)"};const char* en[]={"Attack","Decay","Sustain","Release","Pitch amount"};
        for(int n=0;n<2;++n){int x=n==0?58:284;int off=n*4;text(g,n==0?"AMPLITUDE":tr("HAUTEUR","PITCH"),{x,359,215,27},15,ink,true);
            envelopeGraph(g,{(float)x+9,400,187,46},globalEnv[(size_t)off].getValue(),globalEnv[(size_t)off+1].getValue(),globalEnv[(size_t)off+2].getValue(),globalEnv[(size_t)off+3].getValue());
            for(int k=0;k<(n==0?4:5);++k){int i=off+k;int yy=470+k*37;text(g,tr(fr[k],en[k]),{x,yy,130,20},13,muted);text(g,juce::String(globalEnv[(size_t)i].getValue(),k==2?3:3)+(k==2?"":k==4?" st":" s"),{x+130,yy,76,20},12,ink,true,juce::Justification::centredRight);}}
        for(int j=0;j<3;++j){int op=envPage*3+j,x=531+j*318; text(g,tr("OPÉRATEUR ","OPERATOR ")+juce::String(op+1),{x,354,190,27},13,ink,true);if(j<2)text(g,"ADSR",{x+240,354,42,27},13,muted,true);
            int k0=op*4;envelopeGraph(g,{(float)x+77,385,126,32},opEnv[(size_t)k0].getValue(),opEnv[(size_t)k0+1].getValue(),opEnv[(size_t)k0+2].getValue(),opEnv[(size_t)k0+3].getValue());
            for(int k=0;k<4;++k){int xx=x+(k%2)*150,yy=430+(k/2)*46;text(g,tr(fr[k],en[k]),{xx,yy,75,18},11,muted);text(g,juce::String(opEnv[(size_t)(k0+k)].getValue(),3)+(k==2?"":" s"),{xx+75,yy,60,18},11,ink,true,juce::Justification::centredRight);}}
    }
    void drawEffects(juce::Graphics& g){
        for(int x:{513,990}){g.setColour(edge);g.drawVerticalLine(x,343,766);}
        const char* titles[]={"CHORUS","DELAY","REVERB"};
        const char* fr[]={"Mouvement stéréo · voix déphasées","Écho stéréo · temps modulable","Réverbération diffuse · espace et profondeur"};
        const char* en[]={"Stereo motion · phase-offset voices","Stereo echo · smooth time modulation","Diffused reverb · space and controlled tails"};
        for(int i=0;i<3;++i){text(g,titles[i],{68+i*477,367,320,29},21,ink);text(g,tr(fr[i],en[i]),{68+i*477,410,400,29},14,muted);}
        const char* f[]={"Mix","Vitesse","Profondeur","Mix","Temps","Feedback","Mix","Déclin","Taille","Amortissement"};
        const char* e[]={"Mix","Rate","Depth","Mix","Time","Feedback","Mix","Decay","Size","Damping"};
        const int xs[]={166,382,166,645,860,645,1123,1340,1123,1340};
        for(int i=0;i<10;++i)text(g,tr(f[i],e[i]),{xs[i]-80,(i==2||i==5||i>=8)?625:478,160,25},14,muted,false,juce::Justification::centred);
    }
    void drawPresets(juce::Graphics& g){
        g.setColour(edge);g.drawVerticalLine(316,343,766);g.drawVerticalLine(1204,343,766);
        for(int i=0;i<6;++i){int c=catPage*6+i;if(c>=13)continue;auto r=categoryHits[(size_t)i].getBounds().toFloat();if(c==category)frame(g,r,juce::Colour(0xff2b485a),juce::Colour(0xff4c7793),2);
            text(g,cat(c),{63,(int)r.getY()+9,199,23},13,ink);if(c<=10)text(g,c==0?"100":"10",{267,(int)r.getY()+9,21,23},11,muted,false,juce::Justification::centredRight);}
        for(int i=0;i<12;++i){const int k=presetPage*12+i;if(k>=(int)entries.size())break;const auto& e=entries[(size_t)k];auto b=presetHits[(size_t)i].getBounds();auto r=b.withWidth(413).toFloat();bool active=e.factory>=0?userTitle.isEmpty()&&e.factory==synth.getCurrentProgram():e.file==selectedUser;
            frame(g,r,active?juce::Colour(0xff294557):juce::Colour(0xff202d37),active?cyan:edge,2);text(g,e.label,{b.getX()+12,b.getY()+4,355,22},14,ink);text(g,e.factory>=0?cat(e.factory/10+1):cat(12),{b.getX()+12,b.getY()+25,355,18},11,muted);
            if(e.factory>=0){bool f=synth.favourites[(size_t)e.factory].load();text(g,f?utf("★"):utf("☆"),{b.getRight(),b.getY(),35,50},24,f?juce::Colour(0xffeac477):muted,false,juce::Justification::centred);}}
        text(g,tr("VOTRE BIBLIOTHÈQUE","YOUR SOUND LIBRARY"),{1223,364,228,27},15,ink,true);
        text(g,tr("Les sons d’usine sont protégés.","Factory programs are read-only."),{1223,631,230,24},13,muted);
        text(g,tr("Enregistrez vos sons personnels.","Save an edited sound to make it your own."),{1223,654,230,30},13,muted);
        text(g,tr("★ Favoris des programmes d’usine","★ Factory program favourites"),{1223,699,230,26},12,muted);
        text(g,juce::String(presetPage+1)+" / "+juce::String(juce::jmax(1,((int)entries.size()+11)/12)),{1160,739,36,22},11,muted);
    }
    void drawMaster(juce::Graphics& g){
        frame(g,{36,783,1432,150},juce::Colour(0xff242e36),edge);
        g.setColour(edge);g.drawVerticalLine(491,784,932);g.drawVerticalLine(1012,784,932);
        text(g,tr("MOTEUR DE MODULATION","MODULATION ENGINE"),{53,792,430,23},13,muted,true);
        text(g,tr("JEU & MOUVEMENT","PERFORMANCE & MOTION"),{508,792,490,23},13,muted,true);
        text(g,tr("SORTIE MASTER","MASTER OUTPUT"),{1029,792,421,23},13,muted,true);
        const char* fr[]={"Profondeur FM","Profondeur AM","Quantité","Feedback","Vélocité","Suivi clavier","LFO Hz","Vibrato ct","Trémolo","LFO → mod","Transpose","Accord ct","Volume dB","Pan","Largeur"};
        const char* en[]={"FM depth","AM depth","Amount","Feedback","Velocity","Key track","LFO Hz","Vibrato ct","Tremolo","LFO → mod","Transpose","Tune ct","Volume dB","Pan","Width"};
        const int xs[]={102,210,317,424,543,625,707,789,871,953,1068,1154,1240,1326,1412};
        for(int i=0;i<15;++i)text(g,tr(fr[i],en[i]),{xs[i]-43,816,86,22},13,muted,false,juce::Justification::centred);
    }
};
FMEmancipationAudioProcessorEditor::FMEmancipationAudioProcessorEditor(FMEmancipationAudioProcessor& p):juce::AudioProcessorEditor(&p),surface(std::make_unique<Surface>(p)){
    addAndMakeVisible(*surface);setResizable(true,true);setResizeLimits(780,522,1504,1006);if(auto* c=getConstrainer())c->setFixedAspectRatio(1504.0/1006.0);
    int w=1120; if(auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())w=juce::jmin(w,juce::jmin(display->userArea.getWidth()-40,(int)((display->userArea.getHeight()-70)*1504.0/1006.0)));
    w=juce::jmax(780,w);setSize(w,juce::roundToInt(w*1006.0/1504.0));
}
FMEmancipationAudioProcessorEditor::~FMEmancipationAudioProcessorEditor()=default;
void FMEmancipationAudioProcessorEditor::paint(juce::Graphics& g){g.fillAll(juce::Colour(0xff20262c));}
void FMEmancipationAudioProcessorEditor::resized(){if(!surface)return;const float scale=juce::jmin(getWidth()/1504.0f,getHeight()/1006.0f);surface->setTransform(juce::AffineTransform());surface->setBounds(0,0,1504,1006);surface->setTransform(juce::AffineTransform::scale(scale).translated((getWidth()-1504*scale)*0.5f,(getHeight()-1006*scale)*0.5f));}
