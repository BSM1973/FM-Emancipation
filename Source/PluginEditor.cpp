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
    updateLfoSourceVisibility();
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
    setSize(1496, 990);
    setResizable(true, true);
    setResizeLimits(900, 650, 2200, 1500);
    startTimerHz(20); // Refresh only the dual-LFO display area.
}
void FMEmancipationAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    const float scaleX = area.getWidth() / 1496.0f;
    const float scaleY = area.getHeight() / 990.0f;
    g.addTransform(juce::AffineTransform::scale(scaleX, scaleY));
    const auto bg = juce::Colour(0xff20262c);
    const auto blue = juce::Colour(0xff174568);
    const auto line = juce::Colour(0xff465869);
    g.fillAll(bg);
    g.setColour(juce::Colours::whitesmoke);
    g.setFont(juce::FontOptions(38.0f, juce::Font::bold));
    g.drawText("FM", 29, 20, 69, 50, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff78c9fa));
    g.drawText("Emancipation", 92, 20, 360, 50, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff9bafc0));
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("S I X   O P E R A T O R S  ·  I N F I N I T E   E X P R E S S I O N", 30, 78, 560, 24, juce::Justification::left);
    g.setColour(blue); g.fillRoundedRectangle(30, 108, 1432, 143, 4);
    g.setColour(juce::Colour(0xff77bfe6)); g.drawRoundedRectangle(30, 108, 1432, 143, 4, 1);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("P R O G R A M   M E M O R Y", 54, 127, 400, 25, juce::Justification::left);
    g.setFont(juce::FontOptions(29.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffc4e5ff));
    g.drawText("Custom Sound", 54, 158, 530, 45, juce::Justification::left);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("ALGORITHM", 641, 130, 180, 25, juce::Justification::left);
    g.drawText("OPERATOR OUTPUT LEVELS", 1010, 126, 410, 25, juce::Justification::left);
    for (int i = 0; i < 5; ++i) {
        const int x = 30 + i * 192;
        g.setColour(juce::Colour(0xff303b47));
        g.fillRect(x, 270, 182, 44);
        g.setColour(juce::Colour(0xffb5c6d6));
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        const char* tabs[] = { "01   OPERATORS", "02   ALGORITHM", "03   ENVELOPES", "04   EFFECTS", "05   PRESETS" };
        g.drawText(tabs[i], x + 16, 279, 162, 25, juce::Justification::centred);
    }
    for (int i = 0; i < 6; ++i) {
        const int x = 30 + i * 238;
        g.setColour(juce::Colour(0xff29323b)); g.fillRoundedRectangle((float)x, 331, 238, 425, 3);
        g.setColour(line); g.drawRoundedRectangle((float)x, 331, 238, 425, 3, 1);
        g.setColour(juce::Colour(0xffcee5f7));
        g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
        g.drawText("OP " + juce::String(i + 1), x + 18, 352, 120, 30, juce::Justification::left);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(i < 2 ? "CARRIER" : "FM MOD", x + 150, 358, 80, 23, juce::Justification::centred);
        g.drawText("Frequency ratio", x + 18, 395, 195, 20, juce::Justification::left);
        g.drawText("Output level", x + 18, 507, 195, 20, juce::Justification::left);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("FB", x + 4, 527, 58, 17, juce::Justification::centred);
        g.drawText("VEL", x + 154, 527, 80, 17, juce::Justification::centred);
        g.setFont(juce::FontOptions(13.0f));
        // Live ADSR preview, using the same parameter values as the knobs.
        const auto envValue = [&](const juce::String& key) {
            return processor.parameters.getRawParameterValue(key + juce::String(i))->load();
        };
        const float attack = envValue("opAttack");
        const float decay = envValue("opDecay");
        const float sustain = envValue("opSustain");
        const float release = envValue("opRelease");
        const float total = attack + decay + 0.35f + release;
        const float x0 = (float)x + 20.0f, y0 = 691.0f;
        const float width = 198.0f, height = 29.0f;
        const float xa = x0 + width * attack / total;
        const float xd = xa + width * decay / total;
        const float xs = xd + width * 0.35f / total;
        const float top = y0 - height, sustainY = y0 - height * sustain;
        juce::Path envelopePath;
        envelopePath.startNewSubPath(x0, y0);
        envelopePath.lineTo(xa, top);
        envelopePath.lineTo(xd, sustainY);
        envelopePath.lineTo(xs, sustainY);
        envelopePath.lineTo(x0 + width, y0);
        g.setColour(juce::Colour(0xff213b4b));
        g.fillRoundedRectangle(x0 - 3, top - 4, width + 6, height + 9, 4.0f);
        g.setColour(juce::Colour(0xff70d6f5));
        g.strokePath(envelopePath, juce::PathStrokeType(2.0f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        const char* adsrLabels[] = { "A", "D", "S", "R" };
        for (int k = 0; k < 4; ++k)
            g.drawText(adsrLabels[k], x + 18 + 54*k, 693, 48, 13, juce::Justification::centred);
    }
    g.setColour(juce::Colour(0xff29323b)); g.fillRoundedRectangle(30, 772, 1432, 149, 4);
    g.setColour(line); g.drawRoundedRectangle(30, 772, 1432, 149, 4, 1);
    g.setColour(juce::Colour(0xff8ba6bc)); g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("MODULATION ENGINE", 48, 785, 225, 20, juce::Justification::left);
    g.setColour(juce::Colour(0xff78c9fa));
    g.drawText("LFO / MODULATION", 305, 785, 205, 20, juce::Justification::left);
    g.setColour(juce::Colour(0xffb5c6d6));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("ONDE", 311, 812, 116, 17, juce::Justification::centred);
    g.drawText("VITESSE", 447, 812, 105, 17, juce::Justification::centred);
    g.drawText("SYNC", 563, 812, 99, 17, juce::Justification::centred);
    g.drawText("DIVISION", 675, 812, 106, 17, juce::Justification::centred);
    g.drawText("INTENSITE", 795, 812, 99, 17, juce::Justification::centred);
    g.drawText("DESTINATION", 914, 812, 151, 17, juce::Justification::centred);
    g.setColour(juce::Colour(0xff8ba6bc));
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    // Live LFO 1 + LFO 2 preview. The shape and amplitude are read directly
    // from APVTS parameters and the phase cursor from lock-free DSP snapshots.
    const auto drawLfoPreview = [&](int index) {
        const bool second = index == 1;
        const juce::String prefix = second ? "lfo2" : "lfo";
        const auto getParam = [&](const juce::String& key) {
            return processor.parameters.getRawParameterValue(key)->load();
        };
        const int shape = juce::jlimit(0, 3, (int)std::lround(getParam(prefix + "Wave")));
        const float depth = juce::jlimit(0.0f, 1.0f, getParam(prefix + "Depth"));
        const float phase = juce::jlimit(0.0f, 1.0f, processor.getLfoPhaseForDisplay(index));
        const float y = second ? 865.0f : 808.0f;
        const juce::Colour accent = second ? juce::Colour(0xff9cb3ff) : juce::Colour(0xff70d6f5);
        g.setColour(juce::Colour(0xff172b3a));
        g.fillRoundedRectangle(1305.0f, y, 150.0f, 51.0f, 4.0f);
        g.setColour(juce::Colour(0xff405b70));
        g.drawRoundedRectangle(1305.0f, y, 150.0f, 51.0f, 4.0f, 1.0f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(accent);
        g.drawText(second ? "LFO 2" : "LFO 1", 1313, (int)y + 3, 70, 15, juce::Justification::left);
        g.setColour(juce::Colour(0xffc4e5ff));
        g.drawText(juce::String(juce::roundToInt(depth * 100.0f)) + " %",
                   1390, (int)y + 3, 57, 15, juce::Justification::right);

        const float left = 1315.0f, width = 130.0f;
        const float midline = y + 35.0f;
        const float amplitude = 12.0f * depth;
        g.setColour(juce::Colour(0xff37576b));
        g.drawLine(left, midline, left + width, midline, 1.0f);
        juce::Path curve;
        for (int point = 0; point <= 96; ++point) {
            const double cycle = (double)point / 96.0;
            double value = std::sin(juce::MathConstants<double>::twoPi * cycle);
            if (shape == 1) value = 1.0 - 4.0 * std::abs(cycle - 0.5);
            else if (shape == 2) value = 2.0 * cycle - 1.0;
            else if (shape == 3) value = cycle < 0.5 ? 1.0 : -1.0;
            const float x = left + width * (float)cycle;
            const float waveY = midline - amplitude * (float)value;
            if (point == 0) curve.startNewSubPath(x, waveY);
            else curve.lineTo(x, waveY);
        }
        g.setColour(accent);
        g.strokePath(curve, juce::PathStrokeType(1.6f));
        const float cursorX = left + width * phase;
        g.setColour(juce::Colour(0xffe3f6ff).withAlpha(0.8f));
        g.drawLine(cursorX, y + 20.0f, cursorX, y + 48.0f, 1.0f);
    };
    drawLfoPreview(0);
    drawLfoPreview(1);
    g.setColour(juce::Colour(0xff8ba6bc));
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("MASTER OUTPUT", 1315, 785, 146, 20, juce::Justification::left);
    g.drawText("FM EMANCIPATION  /  SIX-OPERATOR FM-AM SYNTHESIZER", 30, 934, 800, 20, juce::Justification::left);
}
void FMEmancipationAudioProcessorEditor::timerCallback()
{
    // The waveform previews live at the right of the modulation section.
    const auto scaled = juce::Rectangle<float>(1304.0f, 806.0f, 154.0f, 112.0f)
        .transformedBy(juce::AffineTransform::scale(getWidth() / 1496.0f,
                                                    getHeight() / 990.0f))
        .getSmallestIntegerContainer();
    repaint(scaled);
}

void FMEmancipationAudioProcessorEditor::resized()
{
    auto fit = [&](juce::Component& s, int x, int y, int w, int h) {
        s.setBounds(juce::Rectangle<int>(x, y, w, h)
            .toFloat().transformedBy(juce::AffineTransform::scale(getWidth()/1496.0f, getHeight()/990.0f)).getSmallestIntegerContainer());
    };
    fit(presetSelector, 54, 163, 270, 39);
    fit(savePresetButton, 332, 163, 130, 39);
    fit(loadPresetButton, 470, 163, 120, 39);
    fit(userPresetSelector, 54, 210, 270, 32);
    fit(presetSearch, 332, 210, 170, 32);
    fit(clearPresetSearch, 510, 210, 80, 32);
    fit(presetCategory, 602, 209, 180, 30);
    fit(algorithm, 795, 151, 94, 79);
    fit(fmDepth, 60, 819, 80, 100);
    fit(amDepth, 164, 819, 80, 100);
    fit(output, 1190, 819, 100, 100);
    fit(lfoWaveform, 311, 846, 116, 31);
    fit(lfoRate, 447, 832, 105, 80);
    fit(lfoSync, 563, 846, 99, 31);
    fit(lfoDivision, 675, 846, 106, 31);
    fit(lfoDepth, 795, 832, 99, 80);
    fit(lfoDestination, 914, 846, 151, 31);
    fit(lfoSourceSelector, 526, 780, 112, 28);
    fit(lfoPhaseModeButton, 662, 780, 146, 28);
    fit(modulationEnvelopesButton, 826, 780, 150, 28);
    fit(filterButton, 992, 780, 102, 28);
    fit(effectsButton, 1102, 780, 83, 28);
    fit(arpeggiatorButton, 1194, 780, 105, 28);
    fit(lfo2Waveform, 311, 846, 116, 31);
    fit(lfo2Rate, 447, 832, 105, 80);
    fit(lfo2Sync, 563, 846, 99, 31);
    fit(lfo2Division, 675, 846, 106, 31);
    fit(lfo2Depth, 795, 832, 99, 80);
    fit(lfo2Destination, 914, 846, 151, 31);
    fit(lfoMatrixButton, 1076, 846, 100, 31);
    for (int i = 0; i < 6; ++i) {
        fit(ratios[i], 43 + 238*i, 420, 209, 88);
        fit(levels[i], 103 + 238*i, 543, 60, 81);
        fit(feedbackControls[i], 36 + 238*i, 545, 58, 79);
        fit(velocityControls[i], 172 + 238*i, 545, 58, 79);
        fit(waveformControls[i], 46 + 238*i, 629, 206, 26);
        fit(opAttacks[i], 34 + 238*i, 707, 52, 47);
        fit(opDecays[i], 88 + 238*i, 707, 52, 47);
        fit(opSustains[i], 142 + 238*i, 707, 52, 47);
        fit(opReleases[i], 196 + 238*i, 707, 52, 47);
    }
}

void FMEmancipationAudioProcessorEditor::updateLfoSourceVisibility()
{
    const bool second = lfoSourceSelector.getSelectedId() == 2;
    lfoRate.setVisible(!second);
    lfoDepth.setVisible(!second);
    lfoWaveform.setVisible(!second);
    lfoSync.setVisible(!second);
    lfoDivision.setVisible(!second);
    lfoDestination.setVisible(!second);
    lfo2Rate.setVisible(second);
    lfo2Depth.setVisible(second);
    lfo2Waveform.setVisible(second);
    lfo2Sync.setVisible(second);
    lfo2Division.setVisible(second);
    lfo2Destination.setVisible(second);
    lfoMatrixButton.setButtonText(second ? "MATRICE 2" : "MATRICE 1");
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
