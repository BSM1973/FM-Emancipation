#include "PluginEditor.h"
#include <algorithm>
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
        g.drawText("Output level", x + 18, 530, 195, 20, juce::Justification::left);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("FB", x + 10, 551, 62, 18, juce::Justification::centred);
        g.drawText("VEL", x + 162, 551, 70, 18, juce::Justification::centred);
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
        const float x0 = (float)x + 20.0f, y0 = 665.0f;
        const float width = 198.0f, height = 53.0f;
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
            g.drawText(adsrLabels[k], x + 18 + 54*k, 672, 48, 16, juce::Justification::centred);
    }
    g.setColour(juce::Colour(0xff29323b)); g.fillRoundedRectangle(30, 772, 1432, 149, 4);
    g.setColour(line); g.drawRoundedRectangle(30, 772, 1432, 149, 4, 1);
    g.setColour(juce::Colour(0xff8ba6bc)); g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("MODULATION ENGINE", 48, 785, 400, 20, juce::Justification::left);
    g.drawText("MASTER OUTPUT", 1030, 785, 400, 20, juce::Justification::left);
    g.drawText("FM EMANCIPATION  /  SIX-OPERATOR FM-AM SYNTHESIZER", 30, 934, 800, 20, juce::Justification::left);
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
    fit(presetCategory, 54, 250, 536, 30);
    fit(algorithm, 795, 151, 94, 79);
    fit(fmDepth, 60, 819, 80, 100);
    fit(amDepth, 164, 819, 80, 100);
    fit(output, 1190, 819, 100, 100);
    for (int i = 0; i < 6; ++i) {
        fit(ratios[i], 43 + 238*i, 420, 209, 88);
        fit(levels[i], 85 + 238*i, 548, 66, 112);
        fit(feedbackControls[i], 38 + 238*i, 573, 58, 65);
        fit(velocityControls[i], 172 + 238*i, 573, 58, 65);
        fit(waveformControls[i], 46 + 238*i, 660, 206, 26);
        fit(opAttacks[i], 34 + 238*i, 689, 52, 62);
        fit(opDecays[i], 88 + 238*i, 689, 52, 62);
        fit(opSustains[i], 142 + 238*i, 689, 52, 62);
        fit(opReleases[i], 196 + 238*i, 689, 52, 62);
    }
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
