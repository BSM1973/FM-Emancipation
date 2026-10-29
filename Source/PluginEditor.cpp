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

FMEmancipationAudioProcessorEditor::FMEmancipationAudioProcessorEditor(FMEmancipationAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    auto add = [&](juce::Slider& s, const juce::String& id, bool vertical) {
        s.setSliderStyle(vertical ? juce::Slider::LinearVertical : juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 20);
        s.setColour(juce::Slider::thumbColourId, juce::Colour(0xff78c9fa));
        s.setColour(juce::Slider::trackColourId, juce::Colour(0xff4f9dd0));
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffc6e9ff));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2a38));
        addAndMakeVisible(s);
        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, id, s));
    };
    add(algorithm, "algorithm", false);
    add(fmDepth, "fmDepth", false);
    add(amDepth, "amDepth", false);
    add(output, "output", false);
    add(lfoRate, "lfoRate", false);
    add(lfoDepth, "lfoDepth", false);
    add(lfo2Rate, "lfo2Rate", false);
    add(lfo2Depth, "lfo2Depth", false);
    auto addLfoCombo = [&](juce::ComboBox& box, const juce::String& parameterID) {
        box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff174568));
        box.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
        box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
        addAndMakeVisible(box);
        lfoComboAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.parameters, parameterID, box));
    };
    lfoWaveform.addItemList({ "Sinus", "Triangle", "Scie", "Carree" }, 1);
    lfoSync.addItemList({ "Libre", "Tempo" }, 1);
    lfoDivision.addItemList({ "1 mesure", "1/2", "1/4", "1/8", "1/16", "1/8 pointee", "1/8 triolet" }, 1);
    lfoDestination.addItemList({ "Desactive", "Vibrato", "Tremolo", "Profondeur FM", "Profondeur AM", "Matrice (6 OP)" }, 1);
    addLfoCombo(lfoWaveform, "lfoWave");
    addLfoCombo(lfoSync, "lfoSync");
    addLfoCombo(lfoDivision, "lfoDivision");
    addLfoCombo(lfoDestination, "lfoTarget");
    lfo2Waveform.addItemList({ "Sinus", "Triangle", "Scie", "Carree" }, 1);
    lfo2Sync.addItemList({ "Libre", "Tempo" }, 1);
    lfo2Division.addItemList({ "1 mesure", "1/2", "1/4", "1/8", "1/16", "1/8 pointee", "1/8 triolet" }, 1);
    lfo2Destination.addItemList({ "Desactive", "Vibrato", "Tremolo", "Profondeur FM", "Profondeur AM", "Matrice (6 OP)" }, 1);
    addLfoCombo(lfo2Waveform, "lfo2Wave");
    addLfoCombo(lfo2Sync, "lfo2Sync");
    addLfoCombo(lfo2Division, "lfo2Division");
    addLfoCombo(lfo2Destination, "lfo2Target");
    lfoSourceSelector.addItem("LFO 1", 1);
    lfoSourceSelector.addItem("LFO 2", 2);
    lfoSourceSelector.setSelectedId(1, juce::dontSendNotification);
    lfoSourceSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff174568));
    lfoSourceSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
    lfoSourceSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
    lfoSourceSelector.onChange = [this] { updateLfoSourceVisibility(); };
    addAndMakeVisible(lfoSourceSelector);
    lfoRate.setTooltip("Vitesse du LFO en Hz lorsque le mode Libre est selectionne");
    lfoDepth.setTooltip("Intensite de la modulation LFO, de 0 a 100 %");
    lfoDivision.setTooltip("Duree d'un cycle synchronise au tempo du DAW");
    lfoDestination.setTooltip("Destination de modulation du LFO");
    lfo2Rate.setTooltip("LFO 2 : vitesse libre en Hz");
    lfo2Depth.setTooltip("LFO 2 : intensite de modulation");
    lfo2Division.setTooltip("LFO 2 : division rythmique synchronisee au DAW");
    lfoMatrixButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff174568));
    lfoMatrixButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
    lfoMatrixButton.setTooltip("Ouvrir la matrice LFO vers les six operateurs");
    lfoMatrixButton.onClick = [this] {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<FMLfoOperatorMatrix>(processor,
                lfoSourceSelector.getSelectedId() == 2 ? 2 : 1),
            getLocalArea(&lfoMatrixButton, lfoMatrixButton.getLocalBounds()), this);
    };
    addAndMakeVisible(lfoMatrixButton);
    lfoPhaseModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff174568));
    lfoPhaseModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
    lfoPhaseModeButton.setTooltip("Phase initiale et declenchement par note pour les deux LFO");
    lfoPhaseModeButton.onClick = [this] {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<FMLfoPhaseModePanel>(processor),
            getLocalArea(&lfoPhaseModeButton, lfoPhaseModeButton.getLocalBounds()), this);
    };
    addAndMakeVisible(lfoPhaseModeButton);
    modulationEnvelopesButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff174568));
    modulationEnvelopesButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
    modulationEnvelopesButton.setTooltip("Ouvrir les deux ADSR independantes de modulation");
    modulationEnvelopesButton.onClick = [this] {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<FMDualModulationEnvelopePanel>(processor),
            getLocalArea(&modulationEnvelopesButton, modulationEnvelopesButton.getLocalBounds()), this);
    };
    addAndMakeVisible(modulationEnvelopesButton);
    filterButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff174568));
    filterButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
    filterButton.setTooltip("Filtre multimode : coupure, résonance et modulation par les enveloppes");
    filterButton.onClick = [this] {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<FMMultimodeFilterPanel>(processor),
            getLocalArea(&filterButton, filterButton.getLocalBounds()), this);
    };
    addAndMakeVisible(filterButton);
    effectsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff174568));
    effectsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
    effectsButton.setTooltip("Chorus, delay stéréo / ping-pong et réverbération");
    effectsButton.onClick = [this] {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<FMStereoEffectsPanel>(processor),
            getLocalArea(&effectsButton, effectsButton.getLocalBounds()), this);
    };
    addAndMakeVisible(effectsButton);
    arpeggiatorButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff174568));
    arpeggiatorButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
    arpeggiatorButton.setTooltip("Ouvrir l'arpégiateur MIDI synchronisé au tempo");
    arpeggiatorButton.onClick = [this] {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<FMArpeggiatorPanel>(processor),
            getLocalArea(&arpeggiatorButton, arpeggiatorButton.getLocalBounds()), this);
    };
    addAndMakeVisible(arpeggiatorButton);
    // Page visibility is initialized by selectTab(0), once all embedded
    // components and parameter-attached controls have been constructed.
    for (int i = 0; i < 6; ++i) {
        add(ratios[i], "ratio" + juce::String(i), false);
        add(levels[i], "level" + juce::String(i), true);
        add(feedbackControls[i], "feedback" + juce::String(i), false);
        add(velocityControls[i], "velocity" + juce::String(i), false);
        feedbackControls[i].setTooltip("Feedback de l'operateur");
        velocityControls[i].setTooltip("Sensibilite a la velocite MIDI");
        auto& wave = waveformControls[i];
        wave.addItem("Sinus", 1);
        wave.addItem("Triangle", 2);
        wave.addItem("Dent de scie", 3);
        wave.addItem("Carree", 4);
        wave.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff174568));
        wave.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
        wave.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
        wave.setTooltip("Forme d'onde de l'operateur");
        addAndMakeVisible(wave);
        waveformAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "wave" + juce::String(i), wave));
        add(opAttacks[i], "opAttack" + juce::String(i), false);
        add(opDecays[i], "opDecay" + juce::String(i), false);
        add(opSustains[i], "opSustain" + juce::String(i), false);
        add(opReleases[i], "opRelease" + juce::String(i), false);
        for (auto* slider : { &opAttacks[i], &opDecays[i], &opSustains[i], &opReleases[i] })
            slider->onValueChange = [this] { repaint(); };
    }
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetSelector.addItem(processor.getProgramName(i), i + 1);
    presetSelector.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetSelector.onChange = [this] {
        const int index = presetSelector.getSelectedId() - 1;
        if (index >= 0) {
            processor.setCurrentProgram(index);
            repaint();
        }
    };
    presetSelector.setTooltip("100 presets d\u0027usine - 10 familles de sons");
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1b2a38));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
    presetSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
    addAndMakeVisible(presetSelector);
    savePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff174568));
    savePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
    savePresetButton.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Enregistrer un preset FM Emancipation", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile("FM Emancipation").getChildFile("Mon preset.fme"), "*.fme");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
            [this, chooser](const juce::FileChooser& fc) {
                auto target = fc.getResult();
                if (target == juce::File()) return;
                target = target.withFileExtension(".fme");
                if (!target.getParentDirectory().createDirectory()) return;
                juce::MemoryBlock data;
                processor.getStateInformation(data);
                if (!target.replaceWithData(data.getData(), data.getSize()))
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                        "FM Emancipation", "Impossible d'enregistrer le preset.", "OK");
                else
                    refreshUserPresets();
            });
    };
    addAndMakeVisible(savePresetButton);
    loadPresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff174568));
    loadPresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
    loadPresetButton.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Charger un preset FM Emancipation",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile("FM Emancipation"), "*.fme");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) {
                const auto source = fc.getResult();
                if (source == juce::File()) return;
                juce::MemoryBlock data;
                if (!source.loadFileAsData(data) || data.isEmpty()) {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                        "FM Emancipation", "Impossible de lire ce fichier preset.", "OK");
                    return;
                }
                processor.setStateInformation(data.getData(), (int)data.getSize());
                presetSelector.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
                repaint();
            });
    };
    addAndMakeVisible(loadPresetButton);
    userPresetSelector.setTextWhenNothingSelected("MES PRESETS");
    userPresetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1b2a38));
    userPresetSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffc4e5ff));
    userPresetSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77bfe6));
    userPresetSelector.onChange = [this] {
        const auto id = userPresetSelector.getSelectedId();
        if (id <= 0) return;
        if (id > filteredPresetFiles.size()) return;
        juce::MemoryBlock data;
        if (filteredPresetFiles[id - 1].loadFileAsData(data) && !data.isEmpty()) {
            processor.setStateInformation(data.getData(), (int)data.getSize());
            presetSelector.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
            repaint();
        }
    };
    addAndMakeVisible(userPresetSelector);
    presetSearch.setTextToShowWhenEmpty("RECHERCHER UN PRESET...", juce::Colours::lightgrey);
    presetSearch.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1b2a38));
    presetSearch.setColour(juce::TextEditor::textColourId, juce::Colour(0xffc4e5ff));
    presetSearch.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff77bfe6));
    presetSearch.onTextChange = [this] { refreshUserPresets(); };
    addAndMakeVisible(presetSearch);
    clearPresetSearch.onClick = [this] { presetSearch.clear(); presetCategory.setSelectedId(1); refreshUserPresets(); };
    addAndMakeVisible(clearPresetSearch);
    presetCategory.onChange = [this] { refreshUserPresets(); };
    addAndMakeVisible(presetCategory);
    refreshUserPresets();

    // Turn the five graphical mock-up tabs into real interactive controls.
    static constexpr const char* tabNames[] {
        "01   OPERATORS", "02   ALGORITHM", "03   ENVELOPES",
        "04   EFFECTS", "05   PRESETS"
    };
    for (int i = 0; i < 5; ++i) {
        auto& tab = navigationTabs[(size_t)i];
        tab.setButtonText(tabNames[i]);
        tab.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff303b47));
        tab.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4d4e3));
        tab.onClick = [this, i] { selectTab(i); };
        addAndMakeVisible(tab);
    }
    const auto setupSubTab = [this](juce::TextButton& tab, const juce::String& title,
                                    int section, int index) {
        tab.setButtonText(title);
        tab.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff303b47));
        tab.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc4e5ff));
        tab.onClick = [this, section, index] {
            if (section == 1) currentAlgorithmPage = index;
            else if (section == 2) currentEnvelopePage = index;
            else currentEffectPage = index;
            updateTabVisibility();
            repaint();
        };
        addAndMakeVisible(tab);
    };
    setupSubTab(algorithmTabs[0], "ALGORITHMES FM / AM", 1, 0);
    setupSubTab(algorithmTabs[1], "ARPÉGIATEUR MIDI", 1, 1);
    setupSubTab(envelopeTabs[0], "ADSR DES OPÉRATEURS", 2, 0);
    setupSubTab(envelopeTabs[1], "ENV 1 / ENV 2 + LFO", 2, 1);
    setupSubTab(effectTabs[0], "CHORUS / DELAY / RÉVERB", 3, 0);
    setupSubTab(effectTabs[1], "FILTRE MULTIMODE", 3, 1);

    // Keep all old JUCE parameter attachments; move the actual effects,
    // filter, modulation ADSRs and arpeggiator into the relevant real page.
    studioEffectsPage = std::make_unique<FMStereoEffectsPanel>(processor);
    multimodeFilterPage = std::make_unique<FMMultimodeFilterPanel>(processor);
    modulationEnvelopePage = std::make_unique<FMDualModulationEnvelopePanel>(processor);
    arpeggiatorPage = std::make_unique<FMArpeggiatorPanel>(processor);
    addChildComponent(*studioEffectsPage);
    addChildComponent(*multimodeFilterPage);
    addChildComponent(*modulationEnvelopePage);
    addChildComponent(*arpeggiatorPage);
    selectTab(0);
    setSize(1496, 990);
    setResizable(true, true);
    setResizeLimits(900, 650, 2200, 1500);
    startTimerHz(20); // Refresh only the dual-LFO display area.
}
void FMEmancipationAudioProcessorEditor::paint(juce::Graphics& g)
{
    const float sx = (float)getWidth() / 1496.0f;
    const float sy = (float)getHeight() / 990.0f;
    g.addTransform(juce::AffineTransform::scale(sx, sy));
    const auto background = juce::Colour(0xff20262c);
    const auto panel = juce::Colour(0xff29323b);
    const auto border = juce::Colour(0xff465869);
    const auto accent = juce::Colour(0xff78c9fa);
    const auto light = juce::Colour(0xffc4e5ff);
    g.fillAll(background);

    // The navigation controls are real JUCE buttons placed on top of this
    // background; the five header labels are no longer static paint.
    g.setColour(juce::Colours::whitesmoke);
    g.setFont(juce::FontOptions(36.0f, juce::Font::bold));
    g.drawText("FM", 36, 78, 62, 49, juce::Justification::centredLeft);
    g.setColour(accent);
    g.drawText("Emancipation", 100, 78, 350, 49, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff93b2c9));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("SIX OPERATOR FM / AM SYNTHESIZER", 39, 129, 557, 22,
               juce::Justification::left);
    g.setColour(juce::Colour(0xff233542));
    g.fillRoundedRectangle(1266.0f, 90.0f, 193.0f, 104.0f, 5.0f);
    g.setColour(border);
    g.drawRoundedRectangle(1266.0f, 90.0f, 193.0f, 104.0f, 5.0f, 1.0f);
    g.setColour(light);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("MASTER OUTPUT", 1283, 98, 157, 18, juce::Justification::centred);
    g.setColour(juce::Colour(0xff344451));
    g.drawHorizontalLine(174, 34.0f, 1248.0f);

    static constexpr const char* pageTitles[] {
        "SIX OPÉRATEURS", "ARCHITECTURE FM / AM", "ENVELOPPES ET MODULATION",
        "EFFETS ET FILTRAGE", "BIBLIOTHÈQUE DE PRESETS"
    };
    static constexpr const char* pageDescriptions[] {
        "Fréquence, forme d'onde, niveau, feedback et vélocité de chaque opérateur.",
        "Vingt algorithmes de synthèse, profondeur FM, AM et arpégiateur musical.",
        "Six enveloppes ADSR, deux enveloppes de modulation et deux LFO.",
        "Chorus, delay stéréo, réverbération et filtre multimode.",
        "Cent presets d'usine et bibliothèque de sons utilisateur."
    };
    g.setColour(accent);
    g.setFont(juce::FontOptions(23.0f, juce::Font::bold));
    g.drawText(pageTitles[currentTab], 38, 191, 1100, 32, juce::Justification::left);
    g.setColour(juce::Colour(0xffa8bfd1));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(pageDescriptions[currentTab], 39, 224, 1150, 21, juce::Justification::left);

    if (currentTab == 0) {
        // All synthesis operator controls live exclusively on OPERATORS.
        for (int i = 0; i < 6; ++i) {
            const int x = 30 + i * 238;
            g.setColour(panel);
            g.fillRoundedRectangle((float)x, 278.0f, 238.0f, 603.0f, 5.0f);
            g.setColour(border);
            g.drawRoundedRectangle((float)x, 278.0f, 238.0f, 603.0f, 5.0f, 1.0f);
            g.setColour(light);
            g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
            g.drawText("OP " + juce::String(i + 1), x + 20, 294, 128, 34, juce::Justification::left);
            g.setColour(juce::Colour(0xff78c9fa));
            g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            g.drawText(i < 2 ? "CARRIER" : "FM MOD", x + 147, 302, 80, 20,
                       juce::Justification::centred);
            g.setColour(light);
            g.setFont(juce::FontOptions(13.0f));
            g.drawText("FREQUENCY RATIO", x + 19, 356, 200, 21, juce::Justification::centred);
            g.drawText("OUTPUT LEVEL", x + 19, 515, 200, 22, juce::Justification::centred);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText("FEEDBACK", x + 10, 554, 80, 18, juce::Justification::centred);
            g.drawText("LEVEL", x + 84, 554, 73, 18, juce::Justification::centred);
            g.drawText("VELOCITY", x + 160, 554, 71, 18, juce::Justification::centred);
            g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            g.drawText("WAVEFORM", x + 24, 694, 190, 20, juce::Justification::centred);
            g.setColour(juce::Colour(0xff385366));
            g.drawHorizontalLine(770, (float)x + 15.0f, (float)x + 224.0f);
            g.setColour(juce::Colour(0xff8cb5ce));
            g.setFont(juce::FontOptions(12.0f));
            g.drawText("OSCILLATOR " + juce::String(i + 1), x + 18, 787, 200, 24,
                       juce::Justification::centred);
        }
    }
    else if (currentTab == 1 && currentAlgorithmPage == 0) {
        // A live routing diagram shows the actual six-operator carrier path.
        g.setColour(panel);
        g.fillRoundedRectangle(34.0f, 296.0f, 1428.0f, 605.0f, 7.0f);
        g.setColour(border);
        g.drawRoundedRectangle(34.0f, 296.0f, 1428.0f, 605.0f, 7.0f, 1.0f);
        g.setColour(light);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("ALGORITHME", 82, 322, 220, 21, juce::Justification::left);
        g.drawText("FM DEPTH", 297, 322, 190, 21, juce::Justification::centred);
        g.drawText("AM DEPTH", 522, 322, 190, 21, juce::Justification::centred);
        g.setColour(accent);
        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText("SCHÉMA DE CONNEXION DES SIX OPÉRATEURS", 78, 457, 1240, 33,
                   juce::Justification::left);

        static constexpr int routes[20][6] {
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
        const int algorithmIndex = juce::jlimit(0, 19, (int)std::lround(
            processor.parameters.getRawParameterValue("algorithm")->load()) - 1);
        constexpr float cardX[6] { 235.0f, 655.0f, 1075.0f, 235.0f, 655.0f, 1075.0f };
        constexpr float cardY[6] { 527.0f, 527.0f, 527.0f, 732.0f, 732.0f, 732.0f };
        for (int op = 0; op < 6; ++op) {
            const int destination = routes[algorithmIndex][op];
            if (destination < 0) continue;
            juce::Path wire;
            const float sx0 = cardX[op] + 78.0f, sy0 = cardY[op] + 30.0f;
            const float dx0 = cardX[destination] + 78.0f, dy0 = cardY[destination] + 30.0f;
            wire.startNewSubPath(sx0, sy0);
            wire.quadraticTo((sx0 + dx0) * 0.5f, (sy0 + dy0) * 0.5f - 54.0f, dx0, dy0);
            g.setColour(juce::Colour(0xff557fa0));
            g.strokePath(wire, juce::PathStrokeType(2.5f));
        }
        for (int op = 0; op < 6; ++op) {
            const bool carrier = routes[algorithmIndex][op] < 0;
            g.setColour(carrier ? juce::Colour(0xff1b5672) : juce::Colour(0xff30475b));
            g.fillRoundedRectangle(cardX[op], cardY[op], 156.0f, 62.0f, 7.0f);
            g.setColour(carrier ? accent : border);
            g.drawRoundedRectangle(cardX[op], cardY[op], 156.0f, 62.0f, 7.0f, 1.0f);
            g.setColour(light);
            g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
            g.drawText("OP " + juce::String(op + 1), (int)cardX[op] + 15,
                       (int)cardY[op] + 6, 125, 29, juce::Justification::centred);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(carrier ? "CARRIER" : "MODULATEUR",
                       (int)cardX[op] + 9, (int)cardY[op] + 36, 138, 18,
                       juce::Justification::centred);
        }
        g.setColour(juce::Colour(0xff94bad4));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Les connexions suivent l'algorithme sélectionné. Les blocs CARRIER produisent le son.",
                   72, 863, 1250, 23, juce::Justification::centred);
    }
    else if (currentTab == 1) {
        g.setColour(panel);
        g.fillRoundedRectangle(34.0f, 295.0f, 1428.0f, 599.0f, 7.0f);
        g.setColour(border);
        g.drawRoundedRectangle(34.0f, 295.0f, 1428.0f, 599.0f, 7.0f, 1.0f);
    }
    else if (currentTab == 2 && currentEnvelopePage == 0) {
        // The six original ADSR knobs remain attached to their true OP params.
        for (int i = 0; i < 6; ++i) {
            const int x = 30 + 238 * i;
            g.setColour(panel);
            g.fillRoundedRectangle((float)x, 290.0f, 238.0f, 595.0f, 5.0f);
            g.setColour(border);
            g.drawRoundedRectangle((float)x, 290.0f, 238.0f, 595.0f, 5.0f, 1.0f);
            g.setColour(light);
            g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
            g.drawText("OP " + juce::String(i + 1), x + 20, 315, 190, 34, juce::Justification::left);
            g.setColour(accent);
            g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            g.drawText("ADSR / ENVELOPPE D'AMPLITUDE", x + 12, 365, 213, 24,
                       juce::Justification::centred);
            const auto value = [&](const juce::String& name) {
                return processor.parameters.getRawParameterValue(name + juce::String(i))->load();
            };
            const float a = value("opAttack"), d = value("opDecay");
            const float s = value("opSustain"), r = value("opRelease");
            const float span = a + d + 0.5f + r;
            const float left = (float)x + 22.0f, baseline = 527.0f, width = 194.0f, height = 104.0f;
            const float attackX = left + width * a / span;
            const float decayX = attackX + width * d / span;
            const float sustainX = decayX + width * 0.5f / span;
            juce::Path curve;
            curve.startNewSubPath(left, baseline);
            curve.lineTo(attackX, baseline - height);
            curve.lineTo(decayX, baseline - height * s);
            curve.lineTo(sustainX, baseline - height * s);
            curve.lineTo(left + width, baseline);
            g.setColour(juce::Colour(0xff1b3446));
            g.fillRoundedRectangle(left - 5.0f, baseline - height - 9.0f,
                                   width + 10.0f, height + 20.0f, 5.0f);
            g.setColour(accent);
            g.strokePath(curve, juce::PathStrokeType(2.5f));
            static constexpr const char* labels[] { "A", "D", "S", "R" };
            g.setColour(light);
            g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            for (int j = 0; j < 4; ++j)
                g.drawText(labels[j], x + 10 + j * 56, 555, 52, 22, juce::Justification::centred);
            g.setColour(juce::Colour(0xff8ba6bc));
            g.drawText("COURBE LIVE", x + 22, 754, 196, 22, juce::Justification::centred);
        }
    }
    else if (currentTab == 2) {
        g.setColour(panel);
        g.fillRoundedRectangle(30.0f, 735.0f, 1432.0f, 186.0f, 5.0f);
        g.setColour(border);
        g.drawRoundedRectangle(30.0f, 735.0f, 1432.0f, 186.0f, 5.0f, 1.0f);
        g.setColour(accent);
        g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
        g.drawText("LFO / MODULATION", 47, 745, 336, 25, juce::Justification::left);
        g.setColour(light);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        static constexpr const char* labels[] {
            "ONDE", "VITESSE", "SYNC", "DIVISION", "INTENSITÉ", "DESTINATION"
        };
        constexpr int x[] { 311, 447, 563, 675, 795, 914 };
        constexpr int w[] { 116, 105, 99, 106, 99, 151 };
        for (int i = 0; i < 6; ++i)
            g.drawText(labels[i], x[i], 802, w[i], 19, juce::Justification::centred);
        for (int index = 0; index < 2; ++index) {
            const bool second = index != 0;
            const juce::String prefix = second ? "lfo2" : "lfo";
            const auto param = [&](const juce::String& suffix) {
                return processor.parameters.getRawParameterValue(prefix + suffix)->load();
            };
            const int shape = juce::jlimit(0, 3, (int)std::lround(param("Wave")));
            const float depth = juce::jlimit(0.0f, 1.0f, param("Depth"));
            const float phase = juce::jlimit(0.0f, 1.0f,
                                            processor.getLfoPhaseForDisplay(index));
            const float y = second ? 851.0f : 790.0f, left = 1317.0f, width = 125.0f;
            const auto color = second ? juce::Colour(0xff9cb3ff) : accent;
            g.setColour(juce::Colour(0xff172b3a));
            g.fillRoundedRectangle(left - 10.0f, y, 145.0f, 54.0f, 4.0f);
            g.setColour(color);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText(second ? "LFO 2" : "LFO 1", (int)left, (int)y, 124, 17,
                       juce::Justification::left);
            juce::Path waveform;
            for (int i = 0; i <= 80; ++i) {
                const double cycle = (double)i / 80.0;
                double v = std::sin(juce::MathConstants<double>::twoPi * cycle);
                if (shape == 1) v = 1.0 - 4.0 * std::abs(cycle - 0.5);
                else if (shape == 2) v = 2.0 * cycle - 1.0;
                else if (shape == 3) v = cycle < 0.5 ? 1.0 : -1.0;
                const float px = left + width * (float)cycle;
                const float py = y + 35.0f - 11.0f * depth * (float)v;
                if (i == 0) waveform.startNewSubPath(px, py);
                else waveform.lineTo(px, py);
            }
            g.strokePath(waveform, juce::PathStrokeType(1.4f));
            const float cursor = left + width * phase;
            g.setColour(juce::Colour(0xffe4f7ff));
            g.drawLine(cursor, y + 19.0f, cursor, y + 49.0f, 1.0f);
        }
    }
    else if (currentTab == 3) {
        g.setColour(panel);
        g.fillRoundedRectangle(34.0f, 294.0f, 1428.0f, 620.0f, 6.0f);
        g.setColour(border);
        g.drawRoundedRectangle(34.0f, 294.0f, 1428.0f, 620.0f, 6.0f, 1.0f);
    }
    else if (currentTab == 4) {
        g.setColour(panel);
        g.fillRoundedRectangle(43.0f, 281.0f, 672.0f, 594.0f, 7.0f);
        g.fillRoundedRectangle(750.0f, 281.0f, 703.0f, 594.0f, 7.0f);
        g.setColour(border);
        g.drawRoundedRectangle(43.0f, 281.0f, 672.0f, 594.0f, 7.0f, 1.0f);
        g.drawRoundedRectangle(750.0f, 281.0f, 703.0f, 594.0f, 7.0f, 1.0f);
        g.setColour(accent);
        g.setFont(juce::FontOptions(21.0f, juce::Font::bold));
        g.drawText("FACTORY PRESETS", 74, 312, 562, 34, juce::Justification::left);
        g.drawText("USER PRESETS", 779, 312, 572, 34, juce::Justification::left);
        g.setColour(juce::Colour(0xffa2bdd1));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText("100 programmes organisés en dix familles.", 74, 349, 560, 24,
                   juce::Justification::left);
        g.drawText("Rechercher, filtrer, charger et sauvegarder vos sons.", 779, 349, 620, 24,
                   juce::Justification::left);
        g.setColour(border);
        g.drawHorizontalLine(510, 78.0f, 676.0f);
        g.drawHorizontalLine(555, 779.0f, 1420.0f);
        g.setColour(light);
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        g.drawText("PROGRAMME ACTUEL", 80, 530, 570, 25, juce::Justification::left);
        g.setColour(accent);
        g.setFont(juce::FontOptions(25.0f, juce::Font::bold));
        g.drawText(processor.getProgramName(processor.getCurrentProgram()),
                   80, 565, 579, 46, juce::Justification::left);
        g.setColour(juce::Colour(0xff91b1c5));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText("Les presets utilisateur utilisent le format .fme.", 780, 586, 624, 22,
                   juce::Justification::left);
        g.drawText("Les paramètres MIDI, les effets et les modulations sont sauvegardés.",
                   780, 612, 633, 42, juce::Justification::left);
    }
    g.setColour(juce::Colour(0xff8ca7bb));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("FM EMANCIPATION  •  BSM / LIBERTY  •  VST3",
               38, 939, 1000, 24, juce::Justification::left);
}
void FMEmancipationAudioProcessorEditor::timerCallback()
{
    // Repaint only the two oscilloscopes while the modulation page is open.
    if (currentTab != 2 || currentEnvelopePage != 1) return;
    const auto area = juce::Rectangle<float>(1300.0f, 786.0f, 165.0f, 124.0f)
        .transformedBy(juce::AffineTransform::scale((float)getWidth()/1496.0f,
                                                   (float)getHeight()/990.0f))
        .getSmallestIntegerContainer();
    repaint(area);
}

void FMEmancipationAudioProcessorEditor::resized()
{
    const float sx = (float)getWidth() / 1496.0f;
    const float sy = (float)getHeight() / 990.0f;
    auto fit = [&](juce::Component& component, int x, int y, int w, int height) {
        component.setBounds(juce::Rectangle<int>(x, y, w, height)
            .toFloat().transformedBy(juce::AffineTransform::scale(sx, sy))
            .getSmallestIntegerContainer());
    };
    auto embed = [&](juce::Component& component, int x, int y, int w, int height) {
        // Preserve each existing component's working native layout and mouse
        // hit-testing while scaling it into the five-page workspace.
        component.setTransform(juce::AffineTransform());
        component.setBounds(0, 0, w, height);
        component.setTransform(juce::AffineTransform::scale(sx, sy)
                                   .translated(x * sx, y * sy));
    };
    for (int i = 0; i < 5; ++i)
        fit(navigationTabs[(size_t)i], 34 + i * 264, 10, 250, 48);
    for (int i = 0; i < 2; ++i) {
        fit(algorithmTabs[(size_t)i], 50 + i * 297, 251, 283, 36);
        fit(envelopeTabs[(size_t)i], 50 + i * 297, 251, 283, 36);
        fit(effectTabs[(size_t)i], 50 + i * 297, 251, 283, 36);
    }
    fit(output, 1311, 119, 108, 77);
    fit(algorithm, 86, 355, 200, 90);
    fit(fmDepth, 305, 355, 194, 90);
    fit(amDepth, 527, 355, 194, 90);

    // Factory and user presets have their own page, not the header.
    fit(presetSelector, 80, 397, 590, 46);
    fit(savePresetButton, 80, 448, 192, 44);
    fit(loadPresetButton, 286, 448, 192, 44);
    fit(userPresetSelector, 781, 395, 631, 43);
    fit(presetSearch, 781, 453, 390, 38);
    fit(clearPresetSearch, 1182, 453, 104, 38);
    fit(presetCategory, 781, 504, 505, 37);

    // Direct oscillator controls on OPERATORS.
    for (int i = 0; i < 6; ++i) {
        const int x = 30 + 238 * i;
        fit(ratios[(size_t)i], x + 26, 386, 186, 97);
        fit(feedbackControls[(size_t)i], x + 18, 577, 62, 92);
        fit(levels[(size_t)i], x + 88, 577, 62, 92);
        fit(velocityControls[(size_t)i], x + 158, 577, 62, 92);
        fit(waveformControls[(size_t)i], x + 24, 725, 190, 35);
        fit(opAttacks[(size_t)i], x + 9, 583, 52, 91);
        fit(opDecays[(size_t)i], x + 65, 583, 52, 91);
        fit(opSustains[(size_t)i], x + 121, 583, 52, 91);
        fit(opReleases[(size_t)i], x + 177, 583, 52, 91);
    }
    // The envelope modulation subpage retains both LFOs and their matrices.
    fit(lfoSourceSelector, 515, 746, 116, 28);
    fit(lfoPhaseModeButton, 647, 746, 145, 28);
    fit(lfoMatrixButton, 1091, 837, 109, 31);
    fit(lfoWaveform, 311, 836, 116, 32);
    fit(lfoRate, 446, 822, 106, 90);
    fit(lfoSync, 562, 836, 99, 32);
    fit(lfoDivision, 675, 836, 106, 32);
    fit(lfoDepth, 793, 822, 99, 90);
    fit(lfoDestination, 913, 836, 152, 32);
    fit(lfo2Waveform, 311, 836, 116, 32);
    fit(lfo2Rate, 446, 822, 106, 90);
    fit(lfo2Sync, 562, 836, 99, 32);
    fit(lfo2Division, 675, 836, 106, 32);
    fit(lfo2Depth, 793, 822, 99, 90);
    fit(lfo2Destination, 913, 836, 152, 32);

    // Old popup launcher buttons are intentionally hidden; their existing
    // editors are now installed as the relevant in-page modules instead.
    embed(*studioEffectsPage, 239, 321, 1000, 570);
    embed(*multimodeFilterPage, 282, 305, 926, 615);
    embed(*modulationEnvelopePage, 236, 294, 1004, 428);
    embed(*arpeggiatorPage, 357, 347, 780, 372);
}

void FMEmancipationAudioProcessorEditor::selectTab(int index)
{
    currentTab = juce::jlimit(0, 4, index);
    updateTabVisibility();
    repaint();
}

void FMEmancipationAudioProcessorEditor::updateTabVisibility()
{
    // Selected state and actual component visibility always remain in sync.
    const auto updateButton = [](juce::TextButton& button, bool active) {
        button.setColour(juce::TextButton::buttonColourId,
                         active ? juce::Colour(0xff174568) : juce::Colour(0xff303b47));
        button.setColour(juce::TextButton::textColourOffId,
                         active ? juce::Colour(0xffe1f4ff) : juce::Colour(0xffbdcedb));
        button.repaint();
    };
    for (int i = 0; i < 5; ++i)
        updateButton(navigationTabs[(size_t)i], currentTab == i);
    for (int i = 0; i < 2; ++i) {
        algorithmTabs[(size_t)i].setVisible(currentTab == 1);
        envelopeTabs[(size_t)i].setVisible(currentTab == 2);
        effectTabs[(size_t)i].setVisible(currentTab == 3);
        updateButton(algorithmTabs[(size_t)i], currentAlgorithmPage == i);
        updateButton(envelopeTabs[(size_t)i], currentEnvelopePage == i);
        updateButton(effectTabs[(size_t)i], currentEffectPage == i);
    }
    const bool operators = currentTab == 0;
    const bool algorithms = currentTab == 1 && currentAlgorithmPage == 0;
    const bool operatorEnvelopes = currentTab == 2 && currentEnvelopePage == 0;
    const bool modulation = currentTab == 2 && currentEnvelopePage == 1;
    const bool presets = currentTab == 4;
    algorithm.setVisible(algorithms);
    fmDepth.setVisible(algorithms);
    amDepth.setVisible(algorithms);
    output.setVisible(true);
    for (int i = 0; i < 6; ++i) {
        ratios[(size_t)i].setVisible(operators);
        levels[(size_t)i].setVisible(operators);
        feedbackControls[(size_t)i].setVisible(operators);
        velocityControls[(size_t)i].setVisible(operators);
        waveformControls[(size_t)i].setVisible(operators);
        opAttacks[(size_t)i].setVisible(operatorEnvelopes);
        opDecays[(size_t)i].setVisible(operatorEnvelopes);
        opSustains[(size_t)i].setVisible(operatorEnvelopes);
        opReleases[(size_t)i].setVisible(operatorEnvelopes);
    }
    const bool lfo2 = lfoSourceSelector.getSelectedId() == 2;
    lfoSourceSelector.setVisible(modulation);
    lfoWaveform.setVisible(modulation && !lfo2);
    lfoRate.setVisible(modulation && !lfo2);
    lfoSync.setVisible(modulation && !lfo2);
    lfoDivision.setVisible(modulation && !lfo2);
    lfoDepth.setVisible(modulation && !lfo2);
    lfoDestination.setVisible(modulation && !lfo2);
    lfo2Waveform.setVisible(modulation && lfo2);
    lfo2Rate.setVisible(modulation && lfo2);
    lfo2Sync.setVisible(modulation && lfo2);
    lfo2Division.setVisible(modulation && lfo2);
    lfo2Depth.setVisible(modulation && lfo2);
    lfo2Destination.setVisible(modulation && lfo2);
    lfoMatrixButton.setVisible(modulation);
    lfoPhaseModeButton.setVisible(modulation);
    lfoMatrixButton.setButtonText(lfo2 ? "MATRICE 2" : "MATRICE 1");

    studioEffectsPage->setVisible(currentTab == 3 && currentEffectPage == 0);
    multimodeFilterPage->setVisible(currentTab == 3 && currentEffectPage == 1);
    modulationEnvelopePage->setVisible(modulation);
    arpeggiatorPage->setVisible(currentTab == 1 && currentAlgorithmPage == 1);

    // Legacy launchers are superseded by embedded pages, never duplicated.
    filterButton.setVisible(false);
    effectsButton.setVisible(false);
    modulationEnvelopesButton.setVisible(false);
    arpeggiatorButton.setVisible(false);

    presetSelector.setVisible(presets);
    savePresetButton.setVisible(presets);
    loadPresetButton.setVisible(presets);
    userPresetSelector.setVisible(presets);
    presetSearch.setVisible(presets);
    clearPresetSearch.setVisible(presets);
    presetCategory.setVisible(presets);
}

void FMEmancipationAudioProcessorEditor::updateLfoSourceVisibility()
{
    updateTabVisibility();
}

void FMEmancipationAudioProcessorEditor::refreshUserPresets()
{
    const auto folder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("FM Emancipation");
    userPresetFiles = folder.findChildFiles(juce::File::findFiles, true, "*.fme");
    std::sort(userPresetFiles.begin(), userPresetFiles.end(), [](const juce::File& a, const juce::File& b) {
        return a.getFullPathName().compareIgnoreCase(b.getFullPathName()) < 0;
    });
    const auto previousCategory = presetCategory.getText();
    juce::StringArray categories;
    categories.add("TOUTES CATEGORIES");
    for (const auto& file : userPresetFiles) {
        auto name = file.getParentDirectory().getRelativePathFrom(folder);
        if (name == "." || name.isEmpty()) name = "Sans categorie";
        categories.addIfNotAlreadyThere(name);
    }
    categories.sort(true);
    presetCategory.clear(juce::dontSendNotification);
    for (int i = 0; i < categories.size(); ++i)
        presetCategory.addItem(categories[i], i + 1);
    const auto categoryIndex = categories.indexOf(previousCategory);
    presetCategory.setSelectedId(categoryIndex >= 0 ? categoryIndex + 1 : categories.indexOf("TOUTES CATEGORIES") + 1,
                                 juce::dontSendNotification);
    const auto activeCategory = presetCategory.getText();
    userPresetSelector.clear(juce::dontSendNotification);
    filteredPresetFiles.clear();
    const auto query = presetSearch.getText().trim();
    for (int i = 0; i < userPresetFiles.size(); ++i) {
        auto category = userPresetFiles[i].getParentDirectory().getRelativePathFrom(folder);
        if (category == "." || category.isEmpty()) category = "Sans categorie";
        if (activeCategory != "TOUTES CATEGORIES" && category != activeCategory) continue;
        const auto label = category + " / " + userPresetFiles[i].getFileNameWithoutExtension();
        if (query.isNotEmpty() && !label.containsIgnoreCase(query)) continue;
        filteredPresetFiles.add(userPresetFiles[i]);
        userPresetSelector.addItem(label, filteredPresetFiles.size());
    }
    userPresetSelector.setTextWhenNothingSelected("MES PRESETS (" + juce::String(filteredPresetFiles.size()) + ")");
}
