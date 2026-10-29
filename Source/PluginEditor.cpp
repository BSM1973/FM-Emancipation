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
    g.drawText("MASTER OUTPUT", 1030, 785, 400, 20, juce::Justification::left);
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
